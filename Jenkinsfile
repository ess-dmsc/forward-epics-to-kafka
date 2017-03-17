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
                -Dflatc=\$DM_ROOT/usr/bin/flatc \
                -Dpath_include_rdkafka=\$DM_ROOT/usr/include \
                -Dpath_lib_rdkafka=\$DM_ROOT/usr/lib \
                -Dpath_include_flatbuffers=\$DM_ROOT/usr/lib \
                -Dpath_include_streaming_data_types=../streaming-data-types"
        }

        stage("Build") {
            sh "make VERBOSE=1"
        }
    }
}
