FROM opencvcourses/opencv-docker:latest 

    RUN apt-get update && \
        apt-get install -y sudo && \
        apt-get install build-essential -y && \
        apt-get install -y vim && \
        apt-get install -y wget && \
        apt-get install -y unzip

    ENV CC=/usr/bin/gcc \
        CXX=/usr/bin/g++

    WORKDIR $HOME/usr/

    RUN mkdir ./library

    CMD bash

    COPY . /usr/src/app2

    WORKDIR /usr/src/app2

    RUN cmake .

    RUN make
