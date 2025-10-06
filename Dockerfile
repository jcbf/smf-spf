# Multi-stage Dockerfile for smf-spf
# Build with: docker build -t smf-spf:latest .

# ============================================================================
# Stage 1: Builder
# ============================================================================
FROM alpine:3.19 AS builder

LABEL maintainer="smf-spf contributors" \
      description="Build stage for smf-spf milter"

# Install build dependencies
RUN apk add --no-cache \
        build-base \
        libspf2-dev \
        libmilter-dev \
        ca-certificates

# Copy source files
WORKDIR /build
COPY Makefile smf-spf.c ./

# Build the binary (with optimizations)
RUN make smf-spf && \
    strip smf-spf && \
    chmod +x smf-spf

# ============================================================================
# Stage 2: Runtime
# ============================================================================
FROM alpine:3.19

LABEL maintainer="smf-spf contributors" \
      description="Lightweight SPF milter for Sendmail/Postfix" \
      version="2.5.2"

# Install runtime dependencies only
RUN apk add --no-cache \
        libspf2 \
        libmilter \
        ca-certificates \
        tini

# Copy binary from builder stage
COPY --from=builder /build/smf-spf /usr/local/bin/smf-spf

# Copy configuration and scripts
COPY smf-spf.conf /etc/mail/smfs/smf-spf.conf
COPY docker/rootfs/etc/services.d/syslog/run /etc/services.d/syslog/run

# Copy documentation
COPY LICENSE COPYING README.md /usr/share/doc/smf-spf/

# Configure for container environment
RUN sed -i -e 's/^#\?Daemonize.*/Daemonize off/' \
           -e 's/^#\?User.*/User nobody/' \
           -e 's/^#\?Socket.*/Socket inet:8890@0.0.0.0/' \
           -e 's/^#\?LogTo.*/LogTo \/dev\/stdout/' \
           -e 's/^#\?Syslog.*/Syslog none/' \
        /etc/mail/smfs/smf-spf.conf \
    && mkdir -p /var/run/smfs \
    && chown -R nobody:nobody /var/run/smfs /etc/mail/smfs \
    && chmod 755 /var/run/smfs \
    && chmod 644 /etc/mail/smfs/smf-spf.conf

# Expose milter port
EXPOSE 8890

# Health check
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD netstat -an | grep -q :8890 || exit 1

# Use tini as init system (proper signal handling)
ENTRYPOINT ["/sbin/tini", "--"]

# Run as nobody user
USER nobody

# Start the milter
CMD ["/usr/local/bin/smf-spf", "-f", "-c", "/etc/mail/smfs/smf-spf.conf"]
