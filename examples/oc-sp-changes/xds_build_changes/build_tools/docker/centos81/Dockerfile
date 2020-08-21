FROM centos:centos8.1.1911
MAINTAINER Andy Fong "andy.fong@stackpath.com"

# RUN groupadd vagrant -g 500
# RUN adduser vagrant -g vagrant -d /home/vagrant -m -s /bin/bash -u 500 
RUN mkdir -p /root/.ssh 

ADD vbatts-bazel-epel-8.repo /etc/yum.repos.d/
ADD ssh/config /root/.ssh/
ADD ssh/id_rsa.jenkins /root/.ssh/ 
RUN chmod 600 /root/.ssh/id_rsa.jenkins

RUN yum -y install epel-release && yum clean all
# RUN yum -y update && yum clean all

# Setup sshd
RUN yum -y install openssh-server 
RUN mkdir /var/run/sshd
RUN echo 'root:password' | chpasswd
RUN sed -i 's/#PermitRootLogin .*/PermitRootLogin yes/' /etc/ssh/sshd_config

# SSH login fix. Otherwise user is kicked off after login
RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd

ENV NOTVISIBLE "in users profile"
RUN echo "export VISIBLE=now" >> /etc/profile
RUN /usr/bin/ssh-keygen -A

RUN yum -y groupinstall "Development Tools"
RUN yum -y install readline-devel which cmake

RUN dnf -y --enablerepo=PowerTools install ninja-build
RUN dnf -y --enablerepo=PowerTools install libstdc++-static

RUN dnf -y install python2
RUN dnf -y install python3
RUN alternatives --set python /usr/bin/python3

RUN dnf -y install go

#llvm-toolset
RUN dnf -y install llvm-toolset

RUN yum -y install bazel
