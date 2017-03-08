node('eee') {
    dir("code") {
        stage("Checkout") {
            checkout scm
            sh "git submodule update --init"
        }
    }

    dir("build") {
        stage("CMake") {
            sh "cmake ../code \
                -Dflatc=\$DM_ROOT/usr/bin/flatc \
                -Dpath_include_rdkafka=\$DM_ROOT/usr/include \
                -Dpath_lib_rdkafka=\$DM_ROOT/usr/lib \
                -Dpath_include_flatbuffers=\$DM_ROOT/usr/lib \
                -Dpath_include_streaming_data_types=../code/streaming-data-types \
                -Dno_graylog_logger=TRUE"
        }

        stage("Build") {
            sh "make VERBOSE=1"
        }
    }
}
