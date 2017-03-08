node('eee') {
    dir("code") {
        stage("Checkout") {
            checkout scm
        }
    }

    dir("build") {
        stage("CMake") {
            sh "cmake ../code \
                -Dflatc=\$DM_ROOT/usr/bin/flatc \
                -Dpath_include_rdkafka=\$DM_ROOT/usr/include \
                -Dpath_lib_rdkafka=\$DM_ROOT/usr/lib \
                -Dpath_include_flatbuffers=\$DM_ROOT/usr/lib
                -Duse_graylog_logger_if_available=FALSE"
        }

        stage("Build") {
            sh "make VERBOSE=1"
        }
    }
}
