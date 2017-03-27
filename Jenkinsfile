node('eee') {
    dir("code") {
        stage("Checkout") {
            checkout scm
        }
    }

    dir("build") {
        stage("make clean") {
            sh "rm -rf ../build/*"
        }

        stage("Update local dependencies") {
            sh "cd .. && bash code/build-script/update-local-deps.sh"
        }

        stage("cmake") {
            sh "bash ../code/build-script/invoke-cmake-from-jenkinsfile.sh"
        }

        stage("Build") {
            sh "make VERBOSE=1"
        }

        stage("Unit Tests") {
            sh "./tests/tests -- --gtest_output=xml"
            junit 'test_detail.xml'
        }

        stage("Archive") {
            archiveArtifacts 'forward-epics-to-kafka'
        }
    }
}
