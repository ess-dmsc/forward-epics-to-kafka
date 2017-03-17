node('eee') {
    dir("code") {
        stage("Checkout") {
            checkout scm
        }
    }

    dir("build") {
        stage("Update streaming-data-types") {
            sh "cd ..; git clone https://github.com/ess-dmsc/streaming-data-types.git; cd streaming-data-types; git pull"
        }

        stage("CMake") {
            sh "cmake ../code \
                -DCMAKE_INCLUDE_PATH=../streaming-data-types;\$DM_ROOT/usr/include;\$DM_ROOT/usr/lib \
                -DCMAKE_LIBRARY_PATH=\$DM_ROOT/usr/lib \
                -Dflatc=\$DM_ROOT/usr/bin/flatc"
        }

        stage("Build") {
            sh "make VERBOSE=1"
        }
    }
}
