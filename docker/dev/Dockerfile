FROM centos:centos7

ARG DEVTOOLSET_VERSION=11

RUN yum install -y centos-release-scl epel-release 

# install devtools and [enable it](https://access.redhat.com/solutions/527703)
# RUN yum install -y \
#     devtoolset-${DEVTOOLSET_VERSION}-gcc \
#     devtoolset-${DEVTOOLSET_VERSION}-gcc-c++ \
#     devtoolset-${DEVTOOLSET_VERSION}-binutils \
#     devtoolset-${DEVTOOLSET_VERSION}-libstdc++-devel
# RUN yum install -y \
#     devtoolset-${DEVTOOLSET_VERSION}-libatomic-devel \
#     devtoolset-${DEVTOOLSET_VERSION}-libasan-devel \
#     devtoolset-${DEVTOOLSET_VERSION}-libubsan-devel \
#     git vim-common wget unzip which java-11-openjdk-devel.x86_64 \
#     libtool autoconf make ninja-build lcov json-devel vim\
#     && yum clean all \
#     && echo "source scl_source enable devtoolset-${DEVTOOLSET_VERSION}" > /etc/profile.d/enable_gcc_toolset.sh 


# RUN wget --no-check-certificate https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.gz \
#     && tar zxf nasm-2.15.05.tar.gz \
#     && cd nasm-2.15.05 \
#     && ./configure \
#     && make install \
#     && rm -rf nasm-2.15.05 \
#     && rm -rf nasm-2.15.05.tar.gz

# install python3-devtools
RUN yum install -y rh-python38-python-devel.x86_64 rh-python38-python-pip.noarch \
    && yum clean all \
    && echo "source scl_source enable rh-python38" > /etc/profile.d/enable_py_toolset.sh
ENV PATH /opt/rh/devtoolset-${DEVTOOLSET_VERSION}/root/usr/bin:/opt/rh/rh-python38/root/usr/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

# install Rust
# RUN curl https://sh.rustup.rs -sSf | sh -s -- -y

# # rust 替换成国内源
# RUN echo -e "\
# [source.crates-io]\n\
# registry = \"https://github.com/rust-lang/crates.io-index\"\n\
# replace-with = \'ustc\'\n\
# [source.ustc]\n\
# registry = \"git://mirrors.ustc.edu.cn/crates.io-index\"\n\
# " > /root/.cargo/config


# install python packages
RUN python -m pip config set global.index-url http://mirrors.aliyun.com/pypi/simple
RUN python -m pip config set global.extra-index-url https://pypi.tuna.tsinghua.edu.cn/simple
RUN python -m pip config set install.trusted-host "mirrors.aliyun.com pypi.tuna.tsinghua.edu.cn"
RUN python3 -m pip install --upgrade pip
# it's not in pip freeze list, but required by setuptools
RUN python3 -m pip install wheel
RUN python3 -m pip install numpy


# ARG BAZEL_FILE=bazel-5.1.1-installer-linux-x86_64.sh

# ARG TOOL_PATH
# RUN echo "haha ${TOOL_PATH}" 
# COPY  tool/${BAZEL_FILE} /tmp
# RUN sh /tmp/${BAZEL_FILE} && rm -f /tmp/${BAZEL_FILE}

# RUN yum remove -y cmake && mkdir /tmp/cmake
# ARG CMAKE_FILE=cmake-3.25.2-linux-x86_64.sh 
# COPY tool/${CMAKE_FILE}  /tmp/cmake

# RUN  cd  /tmp/cmake && sh ${CMAKE_FILE} --skip-license && rm -f ${CMAKE_FILE} \
#     && ln -s /tmp/cmake/bin/cmake /bin/cmake

# ARG GO_VERSION=1.19.7
# ARG GO_SHA256SUM="7a75720c9b066ae1750f6bcc7052aba70fa3813f4223199ee2a2315fd3eb533d"

# Required by grpc
# install go
# RUN set -eux; \
#     url="https://golang.google.cn/dl/go${GO_VERSION}.linux-amd64.tar.gz"; \
#     wget --no-check-certificate -O go.tgz "$url"; \
#     echo "${GO_SHA256SUM} *go.tgz" | sha256sum -c -; \
#     tar -C /usr/local -xzf go.tgz; \
#     rm go.tgz;

# ENV GOPATH="/usr/local"
# ENV PATH="/usr/local/go/bin:${GOPATH}/bin:${PATH}"

WORKDIR /home/admin/

CMD tail -f /dev/null
