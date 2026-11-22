FROM ubuntu:24.04

# Build argument for version (passed from docker-compose.yml)
ARG VERSION=dev

ENV DEBIAN_FRONTEND=noninteractive

# Pinned tool versions
ARG GDB_VERSION=15.0.50.20240403-0ubuntu1
ARG TEMURIN_VERSION=25.0.2.0.0+10-0

# Install core tooling and language runtimes required for the course.
# - C/C++: build-essential, cmake, clang, GCC/G++ 13.3.0 (Ubuntu 24.04 default), gdb 15.0.50 (pinned)
# - Java: Temurin 25.0.2 (installed separately below)
# - Python: 3.12.3 (Ubuntu 24.04 default) + pip, venv
# - SQLite: sqlite3 client and development headers
RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        software-properties-common \
        build-essential \
        cmake \
        clang \
        valgrind \
        gdb=${GDB_VERSION} \
        git \
        wget \
        sudo \
        zsh \
        gnupg2 \
        maven \
        sqlite3 \
        libsqlite3-dev \
        libboost-all-dev \
        python3 \
        python3-venv \
        python3-dev \
        python3-pip \
        tzdata \
        curl && \
    apt-mark hold gdb && \
    rm -rf /var/lib/apt/lists/*

# Install GCC/G++ 13 (13.3.0 on Ubuntu 24.04) and make it the default
RUN apt-get update && \
    apt-get install -y --no-install-recommends gcc-13 g++-13 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100 && \
    rm -rf /var/lib/apt/lists/*

# Install Temurin (Eclipse Adoptium) JDK 25 (pinned)
RUN wget -qO - https://packages.adoptium.net/artifactory/api/gpg/key/public | gpg --dearmor -o /usr/share/keyrings/adoptium.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/adoptium.gpg] https://packages.adoptium.net/artifactory/deb noble main" > /etc/apt/sources.list.d/adoptium.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends temurin-25-jdk=${TEMURIN_VERSION} && \
    apt-mark hold temurin-25-jdk && \
    rm -rf /var/lib/apt/lists/*

# Allow students to 'pip install' without a venv (Ubuntu 24.04 marks system Python as externally managed)
ENV PIP_BREAK_SYSTEM_PACKAGES=1

# Configure JAVA_HOME and PATH for Java tools in an architecture-neutral way
# Create a stable symlink and set env vars
RUN ln -s "$(dirname $(dirname $(readlink -f $(command -v javac))))" /usr/lib/jvm/java-25 && \
    echo 'export JAVA_HOME=/usr/lib/jvm/java-25' > /etc/profile.d/java.sh && \
    echo 'export PATH=$JAVA_HOME/bin:$PATH' >> /etc/profile.d/java.sh
ENV JAVA_HOME=/usr/lib/jvm/java-25
ENV PATH="/usr/lib/jvm/java-25/bin:$PATH"

# Create a non-root user for students
# (remove the default 'ubuntu' user so 'spl' gets UID 1000)
RUN userdel -r ubuntu && \
    useradd -m -s /bin/bash -G sudo spl && \
    echo "spl ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/spl && \
    chmod 0440 /etc/sudoers.d/spl

# Remove the sudo hint message that appears on login
# The message is triggered by /etc/bash.bashrc checking for .sudo_as_admin_successful
# We create this file AND also comment out the sudo hint in /etc/bash.bashrc
RUN touch /home/spl/.sudo_as_admin_successful /home/spl/.hushlogin && \
    chown spl:spl /home/spl/.sudo_as_admin_successful /home/spl/.hushlogin && \
    sed -i '/# sudo hint/,/^fi$/d' /etc/bash.bashrc

# Create welcome message for students - add to .bashrc so it shows in terminals
# Store the version in an environment variable
ENV STUDENT_ENV_VERSION=${VERSION}

RUN echo '' >> /home/spl/.bashrc && \
    echo '# Welcome message' >> /home/spl/.bashrc && \
    echo 'if [ -z "$VSCODE_WELCOME_SHOWN" ]; then' >> /home/spl/.bashrc && \
    echo '  export VSCODE_WELCOME_SHOWN=1' >> /home/spl/.bashrc && \
    echo '  echo ""' >> /home/spl/.bashrc && \
    echo '  echo "═══════════════════════════════════════════════════════════"' >> /home/spl/.bashrc && \
    echo '  echo "  Welcome to BGU SPL Student Env (v${STUDENT_ENV_VERSION})"' >> /home/spl/.bashrc && \
    echo '  echo "═══════════════════════════════════════════════════════════"' >> /home/spl/.bashrc && \
    echo '  echo ""' >> /home/spl/.bashrc && \
    echo '  echo "Available tools:"' >> /home/spl/.bashrc && \
    echo '  echo "  • C/C++: gcc-13, g++-13, gdb, clang, cmake"' >> /home/spl/.bashrc && \
    echo '  echo "  • Java: OpenJDK 25 (Temurin)"' >> /home/spl/.bashrc && \
    echo '  echo "  • Python: 3.12"' >> /home/spl/.bashrc && \
    echo '  echo "  • SQLite: sqlite3"' >> /home/spl/.bashrc && \
    echo '  echo ""' >> /home/spl/.bashrc && \
    echo '  echo "Happy coding! 🚀"' >> /home/spl/.bashrc && \
    echo '  echo ""' >> /home/spl/.bashrc && \
    echo 'fi' >> /home/spl/.bashrc && \
    chown spl:spl /home/spl/.bashrc

USER spl

WORKDIR /home/spl

# Copy small examples into the image so instructors/students can smoke-test the image
COPY --chown=spl:spl examples /home/spl/examples

# Default to an interactive shell; students can mount a workspace when running
CMD ["bash"]
