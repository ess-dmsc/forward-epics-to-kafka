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
                -Dpath_include_flatbuffers=\$DM_ROOT/usr/lib \
                -Dno_graylog_logger=TRUE"
        }

        stage("Build") {
            sh "make VERBOSE=1"
        }
    }
}
