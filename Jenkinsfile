def project = "forward-epics-to-kafka"
def centos = docker.image('essdmscdm/centos-build-node:0.8.0')

def failure_function(exception_obj, failureMessage) {
    def toEmails = [[$class: 'DevelopersRecipientProvider']]
    emailext body: '${DEFAULT_CONTENT}\n\"' + failureMessage + '\"\n\nCheck console output at $BUILD_URL to view the results.', recipientProviders: toEmails, subject: '${DEFAULT_SUBJECT}'
    throw exception_obj
}

node('docker && eee') {
    cleanWs()

    def container_name = "${project}-${env.BRANCH_NAME}-${env.BUILD_NUMBER}"
    def epics_dir = "/opt/epics"
    def epics_profile_file = "/etc/profile.d/ess_epics_env.sh"
    def run_args = "\
        --name ${container_name} \
        --tty \
        --env http_proxy=${env.http_proxy} \
        --env https_proxy=${env.https_proxy} \
        --mount=type=bind,src=${epics_dir},dst=${epics_dir},readonly \
        --mount=type=bind,src=${epics_profile_file},dst=${epics_profile_file},readonly"

    try {
        container = centos.run(run_args)

        stage('Checkout') {
            def checkout_script = """
                git clone https://github.com/ess-dmsc/${project}.git \
                    --branch ${env.BRANCH_NAME}
                git clone -b master https://github.com/ess-dmsc/streaming-data-types.git
            """
            sh "docker exec ${container_name} sh -c \"${checkout_script}\""
        }

        stage('Get Dependencies') {
            def conan_remote = "ess-dmsc-local"
            def dependencies_script = """
                mkdir build
                cd build
                conan remote add \
                    --insert 0 \
                    ${conan_remote} ${local_conan_server}
                conan install ../${project}/conan --build=missing
            """
            sh "docker exec ${container_name} sh -c \"${dependencies_script}\""
        }

        stage('Configure') {
            def configure_script = """
                cd build
                source ${epics_profile_file}
                cmake3 ../${project} -DREQUIRE_GTEST=ON
            """
            sh "docker exec ${container_name} sh -c \"${configure_script}\""
        }

        stage('Build') {
            def build_script = "make --directory=./build VERBOSE=1"
            sh "docker exec ${container_name} sh -c \"${build_script}\""
        }

        stage('Test') {
            def test_output = "TestResults.xml"
            def test_script = """
                cd build
                ./tests/tests -- --gtest_output=xml:${test_output}
            """
            sh "docker exec ${container_name} sh -c \"${test_script}\""

            // Remove file outside container.
            sh "rm -f ${test_output}"
            // Copy and publish test results.
            sh "docker cp ${container_name}:/home/jenkins/build/${test_output} ."

            junit "${test_output}"
        }

        stage('Archive') {
            // Remove file outside container.
            sh "rm -f forward-epics-to-kafka"
            // Copy archive from container.
            sh "docker cp ${container_name}:/home/jenkins/build/forward-epics-to-kafka ."

            archiveArtifacts 'forward-epics-to-kafka'
        }
    } catch (e) {
        failure_function(e, 'Failed to build.')
    } finally {
        container.stop()
    }
}
