FROM alpine:3.14 as snmp-stream

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8
ENV LC_ALL en_US.UTF-8

RUN apk --no-cache add alpine-sdk clang-dev cmake python3-dev openssl-dev doxygen graphviz

RUN apk --no-cache add zsh tmux vim

RUN wget https://raw.githubusercontent.com/sdispater/poetry/master/get-poetry.py -qO - | python3

ENV PATH="/root/.poetry/bin:${PATH}"

COPY . /app

WORKDIR /app
