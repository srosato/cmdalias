FROM ubuntu:18.04

RUN apt-get update
RUN apt-get install -y g++ make bison flex

COPY . /mnt
WORKDIR /mnt

RUN make clean && make && make install && make clean
