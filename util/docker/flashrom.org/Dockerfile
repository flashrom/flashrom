FROM alpine:3.21

RUN apk add --no-cache sphinx py3-sphinx_rtd_theme bash git meson ninja gcc musl-dev pkgconf

VOLUME /data-in /data-out

COPY makeSphinx.sh /makeSphinx.sh

ENV MESON_PACKAGE_CACHE_DIR /tmp/meson-pkgcache/

ENTRYPOINT ["/bin/bash", "/makeSphinx.sh"]
CMD []
