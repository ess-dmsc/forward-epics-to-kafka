def slackFailMsg (msg_str) {
    slackSend color: 'danger', message: '@jonasn E2K (Fast sampling): ' + msg_str
}

node('eee') {
    dir("code") {
        stage("Checkout") {
            try {
                checkout scm
                sh "cd .. && bash code/build-script/update-local-deps.sh"
            } catch (e) {
                slackFailMsg "Checkout failed"
                throw e
            }
        }
    }
    stage("Update dependencies") {
        try {
            sh "bash code/build-script/update-local-deps.sh"
        } catch (e) {
            slackFailMsg "Dependencies failed"
            throw e
        }
    }
    dir("build") {
        stage("Run CMake") {
            try {
                withEnv(["EPICSV4=/opt/epics/modules/"]) {
                    sh "cmake ../code"
                }
            } catch (e) {
                slackFailMsg "CMake failed"
                throw e
            }
        }

        stage("Build") {
            try {
                sh "make"
            } catch (e) {
                slackFailMsg "Build failed"
                throw e
            }
        }
        stage("Run unit tests") {
            try {
                sh "tests/tests --gtest_output=xml:AllResultsUnitTests.xml"
            } catch (e) {
                slackFailMsg "One or more unit tests failed"
            }
            junit '*Tests.xml'
        }
    }
    try {
        if (currentBuild.previousBuild.result == "FAILURE") {
            slackSend color: 'good', message: 'E2K (Fast sampling): Back in the green!'
        }
    } catch (e) {

    }
}
