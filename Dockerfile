FROM opencvcourses/opencv-docker:latest 

    RUN apt-get update && \
        apt-get install -y sudo && \
        apt-get install build-essential -y && \
        apt-get install -y vim && \
        apt-get install -y wget && \
        apt-get install -y unzip

    COPY requirements.txt /usr/src/app/
    RUN pip install --no-cache-dir -r /usr/src/app/requirements.txt

    ENV CC=/usr/bin/gcc \
        CXX=/usr/bin/g++

    WORKDIR $HOME/usr/

    RUN mkdir ./library

    CMD bash

    COPY . /usr/src/app

    WORKDIR /usr/src/app

    RUN cmake .

    RUN make

    EXPOSE 5000

    CMD ["python3", "/usr/src/app/main.py"]
