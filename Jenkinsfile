def project = "forward-epics-to-kafka"
def centos = docker.image('essdmscdm/centos-build-node:0.3.0')
// def fedora = docker.image('essdmscdm/fedora-build-node:0.1.3')
def container_name = "${project}-${env.BRANCH_NAME}-${env.BUILD_NUMBER}"

def conan_remote = "ess-dmsc-local"

node('docker && eee') {
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

        stage('Conan setup') {
            def setup_script = """
                export http_proxy=''
                export https_proxy=''
                conan remote add \
                    --insert 0 \
                    ${conan_remote} ${local_conan_server}
            """
            sh "docker exec ${container_name} sh -c \"${setup_script}\""
        }

        stage('Configure') {
            def configure_script = """
                export http_proxy=''
                export https_proxy=''
                mkdir build
                cd build
                conan install ../${project}/conan \
                    --build=missing
                source ${epics_profile_file}
                cmake3 ../${project} -DREQUIRE_GTEST=ON
            """
            sh "docker exec ${container_name} sh -c \"${configure_script}\""
        }

        stage('Build') {
            def build_script = "make --directory=./build VERBOSE=1"
            sh "docker exec ${container_name} sh -c \"${build_script}\""
        }

        stage('Tests') {
            def test_output = "TestResults.xml"
            def test_script = """
                cd build
                ./tests/tests -- --gtest_output=xml:${test_output}
            """
            sh "docker exec ${container_name} sh -c \"${test_script}\""
            sh "rm -f ${test_output}" // Remove file outside container.
            sh "docker cp ${container_name}:/home/jenkins/build/${test_output} ."
            junit "${test_output}"
        }
    } finally {
        container.stop()
    }
}
