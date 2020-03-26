FROM alpine:latest

COPY Makefile smf-spf.c /tmp/src/

 # Upgrade existing packages & install runtime dependencies
RUN  apk update \
 && apk upgrade \
 && apk add --no-cache \
        libspf2 libmilter \

 # Build smf-spf binary
 && apk add --no-cache --virtual .build-deps \
        build-base \
        libspf2-dev libmilter-dev \
 && cd /tmp/src \
 && make smf-spf \
 && mv smf-spf /usr/local/bin/ \

 # Clean up unnecessary stuff
 && apk del .build-deps \
 && rm -rf /tmp/src \
           /var/cache/apk/*


COPY docker/rootfs /

COPY smf-spf.conf /etc/smfs/

COPY LICENSE COPYING readme README.md /usr/share/doc/smf-spf/

RUN chmod +x /etc/services.d/*/run \

 # Prepare default configuration
 && sed -i -r 's/^#?Daemonize\s.*$/Daemonize off/g' /etc/smfs/smf-spf.conf \
 && sed -i -r 's/^#?User\s.*$/User nobody/g'        /etc/smfs/smf-spf.conf \
 && sed -i -r 's/^#?Socket\s.*$/Socket inet:8890/g' /etc/smfs/smf-spf.conf \

 # Prepare directory for unix socket
 && mkdir -p /var/run/smfs \
 && chown -R nobody:nobody /var/run/smfs


# Install s6-overlay
RUN apk add --update --no-cache --virtual .tool-deps \
        curl \
 && curl -fL -o /tmp/s6-overlay.tar.gz \
         https://github.com/just-containers/s6-overlay/releases/download/v1.21.7.0/s6-overlay-amd64.tar.gz \
 && tar -xzf /tmp/s6-overlay.tar.gz -C / \
    \
 # Cleanup unnecessary stuff
 && apk del .tool-deps \
 && rm -rf /var/cache/apk/* \
           /tmp/*

ENV S6_BEHAVIOUR_IF_STAGE2_FAILS=2 \
    S6_CMD_WAIT_FOR_SERVICES=1


RUN chmod +x /etc/services.d/*/run


EXPOSE 8890


ENTRYPOINT ["/init"]

CMD ["/usr/local/bin/smf-spf"]
