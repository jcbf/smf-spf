#-------------------------------------------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See https://go.microsoft.com/fwlink/?linkid=2090316 for license information.
#-------------------------------------------------------------------------------------------------------------

# To fully customize the contents of this image, use the following Dockerfile instead:
# https://github.com/microsoft/vscode-dev-containers/tree/v0.112.0/containers/ubuntu-18.04-git/.devcontainer/Dockerfile
FROM mcr.microsoft.com/vscode/devcontainers/base:0-ubuntu-18.04

# ** [Optional] Uncomment this section to install additional packages. **
#
ENV LC_ALL en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8
 ENV DEBIAN_FRONTEND=noninteractive
 ENV NODE_TLS_REJECT_UNAUTHORIZED=0
 RUN apt-get update \
    && apt-get -y upgrade \
    && apt-get -y install --no-install-recommends locales build-essential libmilter-dev libspf2-dev opendkim-tools lcov\
    && localedef -i en_US -f UTF-8 en_US.UTF-8 \
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*
 ENV DEBIAN_FRONTEND=dialog


