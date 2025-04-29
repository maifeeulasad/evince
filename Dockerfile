FROM ubuntu:22.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt update -y && \ 
    apt install -y python3-pip python3-dev libpq-dev gcc g++ meson ninja-build \ 
    libcairo2 libpango-1.0-0 libpangocairo-1.0-0 libgdk-pixbuf2.0-0 libffi-dev \ 
    shared-mime-info libglib2.0-dev libhandy-1-dev libexempi-dev itstool \ 
    libsecret-1-dev libsynctex-dev libspectre-dev libarchive-dev gettext \ 
    libnautilus-extension-dev libgirepository1.0-dev libxml2-dev




WORKDIR /eninceapp

COPY . .


RUN rm -rf build

# Build
RUN mkdir build && cd build && \
    meson setup --prefix=/usr --buildtype=release -Dgtk_doc=false --wrap-mode=nodownload -Dsystemduserunitdir=no .. && \
    ninja

# Install
RUN cd build && ninja install

# docker build -t eninceapp . && docker run -it --rm eninceapp