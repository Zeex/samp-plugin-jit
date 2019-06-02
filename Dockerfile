# Usage:
#
# docker build . -t jit
# docker run --rm --name jit -v $PWD/docker/server.cfg:/usr/local/samp03/server.cfg -v $PWD:/usr/local/samp03/plugins/jit -it jit
#
# Build:
#
# mkdir -p build/linux/docker-debug
# cd build/linux/docker-debug
# cmake ../../../ -DCMAKE_BUILD_TYPE=Debug
# make
# cp jit.so $SAMP_SERVER_ROOT/plugins/
#
# Run:
#
# cd $SAMP_SERVER_ROOT
# ./samp03svr

FROM ubuntu

RUN apt-get update -q && \
    apt-get install -y wget vim gcc g++ gcc-multilib g++-multilib make cmake

RUN wget http://files.sa-mp.com/samp037svr_R2-2-1.tar.gz
RUN tar xvzf samp037svr_R2-2-1.tar.gz -C /usr/local/
RUN chmod +x /usr/local/samp03/samp03svr
RUN mkdir /usr/local/samp03/plugins
RUN echo "\noutput 1\n" >> /usr/local/samp03/server.cfg
RUN sed -i -e 's/rcon_password.*$/rcon_password jit/' \
    /usr/local/samp03/server.cfg
RUN rm samp037svr_R2-2-1.tar.gz
ENV SAMP_SERVER_ROOT=/usr/local/samp03

RUN wget https://github.com/pawn-lang/compiler/releases/download/v3.10.9/pawnc-3.10.9-linux.tar.gz
RUN tar xzvf pawnc-3.10.9-linux.tar.gz -C /usr/local --strip-components 1
RUN rm pawnc-3.10.9-linux.tar.gz
RUN ldconfig
ENV PAWNCC_FLAGS="-i$SAMP_SERVER_ROOT/include -(+ -;+"

RUN wget https://github.com/Zeex/plugin-runner/releases/download/v1.1.2/plugin-runner-1.1.2-linux.tar.gz
RUN mkdir /usr/local/plugin-runner
RUN tar xvf plugin-runner-1.1.2-linux.tar.gz \
    --strip-components 1 \
    -C /usr/local/plugin-runner \
    plugin-runner-1.1.2-linux/plugin-runner \
    plugin-runner-1.1.2-linux/include
RUN rm plugin-runner-1.1.2-linux.tar.gz
ENV PATH=$PATH:/usr/local/plugin-runner

WORKDIR $SAMP_SERVER_ROOT/plugins/jit
