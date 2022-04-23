FROM gitpod/workspace-full
USER gitpod

ENV LC_ALL en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8
ENV DEBIAN_FRONTEND=noninteractive
ENV NODE_TLS_REJECT_UNAUTHORIZED=0

RUN sudo apt-get update \
    && sudo apt-get -y upgrade \
    && sudo apt-get -y install --no-install-recommends locales build-essential libmilter-dev libspf2-dev opendkim-tools lcov\
    && sudo localedef -i en_US -f UTF-8 en_US.UTF-8 \
    && sudo apt-get autoremove -y \
    && sudo apt-get clean -y \
    && sudo rm -rf /var/lib/apt/lists/*

ENV DEBIAN_FRONTEND=dialog
