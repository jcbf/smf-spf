# Docker Deployment Guide

## Quick Start

### Using Docker Run

```bash
# Pull the latest image
docker pull underspell/smf-spf:latest

# Run with default configuration
docker run -d \
  --name smf-spf \
  -p 8890:8890 \
  --restart unless-stopped \
  underspell/smf-spf:latest
```

### Using Docker Compose

1. Create a `docker-compose.yml`:

```yaml
version: '3.8'

services:
  smf-spf:
    image: underspell/smf-spf:latest
    container_name: smf-spf
    restart: unless-stopped
    ports:
      - "8890:8890"
    volumes:
      - ./smf-spf.conf:/etc/mail/smfs/smf-spf.conf:ro
```

2. Start the service:

```bash
docker-compose up -d
```

## Building from Source

```bash
# Clone the repository
git clone https://github.com/jcbf/smf-spf.git
cd smf-spf

# Build the image
docker build -t smf-spf:local .

# Run your custom image
docker run -d -p 8890:8890 smf-spf:local
```

## Configuration

### Environment Variables

The Docker image uses these default settings optimized for containers:

- **Daemonize**: `off` (runs in foreground for proper container lifecycle)
- **User**: `nobody` (runs as unprivileged user)
- **Socket**: `inet:8890@0.0.0.0` (listens on all interfaces)
- **LogTo**: `/dev/stdout` (logs to container stdout)
- **Syslog**: `none` (syslog disabled in favor of stdout)

### Custom Configuration

Mount your own configuration file:

```bash
docker run -d \
  --name smf-spf \
  -p 8890:8890 \
  -v /path/to/your/smf-spf.conf:/etc/mail/smfs/smf-spf.conf:ro \
  underspell/smf-spf:latest
```

Example custom `smf-spf.conf`:

```conf
# Whitelist internal networks
WhitelistIP 10.0.0.0/8
WhitelistIP 172.16.0.0/12
WhitelistIP 192.168.0.0/16

# SPF policy
RefuseFail on
TagSubject on
Tag [SPF:fail]

# Authentication-Results header
AddHeader on
AuthservID mail.example.com
```

## Integration with Mail Servers

### Postfix

Add to `/etc/postfix/main.cf`:

```conf
# For Docker host
smtpd_milters = inet:localhost:8890
non_smtpd_milters = inet:localhost:8890

# For Docker network (if Postfix is also containerized)
smtpd_milters = inet:smf-spf:8890
non_smtpd_milters = inet:smf-spf:8890

milter_default_action = accept
milter_protocol = 6
```

### Sendmail

Add to `/etc/mail/sendmail.mc`:

```m4
INPUT_MAIL_FILTER(`smf-spf',
  `S=inet:8890@localhost, F=T, T=S:4m;R:4m;E:10m')dnl
```

## Docker Compose with Postfix

Complete example with Postfix MTA:

```yaml
version: '3.8'

services:
  smf-spf:
    image: underspell/smf-spf:latest
    container_name: smf-spf
    restart: unless-stopped
    networks:
      - mail-network
    volumes:
      - ./smf-spf.conf:/etc/mail/smfs/smf-spf.conf:ro
    healthcheck:
      test: ["CMD", "netstat", "-an", "|", "grep", "-q", ":8890"]
      interval: 30s
      timeout: 5s
      retries: 3

  postfix:
    image: boky/postfix:latest
    container_name: postfix
    restart: unless-stopped
    ports:
      - "25:25"
      - "587:587"
    environment:
      - ALLOWED_SENDER_DOMAINS=example.com
      - SMTPD_MILTERS=inet:smf-spf:8890
      - NON_SMTPD_MILTERS=inet:smf-spf:8890
    networks:
      - mail-network
    depends_on:
      smf-spf:
        condition: service_healthy

networks:
  mail-network:
    driver: bridge
```

## Monitoring and Logs

### View logs

```bash
# Follow logs in real-time
docker logs -f smf-spf

# View last 100 lines
docker logs --tail 100 smf-spf
```

### Health Check

```bash
# Check container health
docker inspect --format='{{.State.Health.Status}}' smf-spf

# Manual health check
docker exec smf-spf netstat -an | grep 8890
```

## Security Considerations

1. **Non-root User**: Container runs as `nobody` (UID 65534)
2. **Read-only Root**: Consider running with `--read-only`:
   ```bash
   docker run -d \
     --name smf-spf \
     -p 8890:8890 \
     --read-only \
     --tmpfs /tmp \
     --tmpfs /var/run/smfs \
     underspell/smf-spf:latest
   ```

3. **Security Options**:
   ```bash
   docker run -d \
     --name smf-spf \
     -p 8890:8890 \
     --security-opt=no-new-privileges:true \
     --cap-drop=ALL \
     underspell/smf-spf:latest
   ```

## Troubleshooting

### Container won't start

```bash
# Check logs for errors
docker logs smf-spf

# Verify configuration syntax
docker run --rm -v $(pwd)/smf-spf.conf:/test.conf:ro \
  underspell/smf-spf:latest \
  /usr/local/bin/smf-spf -f -c /test.conf
```

### MTA can't connect

```bash
# Verify port is exposed
docker port smf-spf

# Test connectivity
telnet localhost 8890

# Check if service is listening
docker exec smf-spf netstat -tlnp
```

### Permission issues

```bash
# Check file ownership
docker exec smf-spf ls -la /etc/mail/smfs/

# Fix permissions on host
chmod 644 smf-spf.conf
```

## Performance Tuning

### Resource Limits

```yaml
services:
  smf-spf:
    image: underspell/smf-spf:latest
    deploy:
      resources:
        limits:
          cpus: '0.5'
          memory: 128M
        reservations:
          cpus: '0.1'
          memory: 64M
```

### Caching

Adjust TTL in configuration for better performance:

```conf
# Cache SPF results for 2 hours
TTL 2h
```

## Multi-Architecture Support

The Docker image supports multiple architectures:
- `linux/amd64`
- `linux/arm64`

Docker automatically pulls the correct architecture for your platform.

## Updates

```bash
# Pull latest image
docker pull underspell/smf-spf:latest

# Recreate container with new image
docker-compose down
docker-compose pull
docker-compose up -d
```
