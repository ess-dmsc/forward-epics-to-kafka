node('eee') {
    dir("code") {
        stage("Checkout") {
            checkout scm
        }
    }

    dir("build") {
        stage("Update local dependencies") {
            sh "bash -c 'cd .. && . code/build-script/update-local-deps.sh'"
        }

        stage("CMake") {
            sh "cmake ../code \
                -DCMAKE_INCLUDE_PATH=../streaming-data-types\;\$DM_ROOT/usr/include\;\$DM_ROOT/usr/lib \
                -DCMAKE_LIBRARY_PATH=\$DM_ROOT/usr/lib \
                -Dflatc=\$DM_ROOT/usr/bin/flatc"
        }

        stage("Build") {
            sh "make VERBOSE=1"
        }
    }
}
