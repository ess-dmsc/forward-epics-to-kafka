def project = "forward-epics-to-kafka"
def centos = docker.image('essdmscdm/centos-build-node:0.3.0')
// def fedora = docker.image('essdmscdm/fedora-build-node:0.1.3')
def container_name = "${project}-${env.BRANCH_NAME}-${env.BUILD_NUMBER}"

def conan_remote = "ess-dmsc-local"

node('docker && eee') {
    def epics_dir = "/opt/epics"
    def run_args = "\
        --name ${container_name} \
        --tty \
        --env http_proxy=${env.http_proxy} \
        --env https_proxy=${env.https_proxy} \
        --env BASE=3.15.4 \
        --env EPICS_BASE=/opt/epics/bases/base-3.15.4 \
        --env EPICS_HOST_ARCH=centos7-x86_64 \
        --env EPICS_DB_INCLUDE_PATH=/opt/epics/bases/base-3.15.4/dbd \
        --env EPICS_MODULES_PATH=/opt/epics/modules \
        --env EPICS_ENV_PATH=/opt/epics/modules/environment/2.0.0/3.15.4/bin/centos7-x86_64 \
        --env EPICS_BASES_PATH=/opt/epics/bases \
        --mount=type=bind,src=${epics_dir},dst=${epics_dir},readonly"

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
                cmake3 ../${project} -DREQUIRE_GTEST=ON
            """
            sh "docker exec ${container_name} sh -c \"${configure_script}\""
        }

        stage('Build') {
            def build_script = "make --directory=./build"
            sh "docker exec ${container_name} sh -c \"${build_script}\""
        }

        // stage('Tests') {
        //     def test_output = "AllResultsUnitTests.xml"
        //     def test_script = """
        //         ./build/unit_tests/unit_tests --gtest_output=xml:${test_output}
        //     """
        //     sh "docker exec ${container_name} sh -c \"${test_script}\""
        //     sh "rm -f ${test_output}" // Remove file outside container.
        //     sh "docker cp ${container_name}:/home/jenkins/${test_output} ."
        //     junit "${test_output}"
        // }
        //
        // stage('Cppcheck') {
        //     def cppcheck_script = "make --directory=./build cppcheck"
        //     sh "docker exec ${container_name} sh -c \"${cppcheck_script}\""
        // }
        //
        // sh "docker cp ${container_name}:/home/jenkins/${project} ./srcs"
    } finally {
        container.stop()
    }

    // try {
    //     container = fedora.run(run_args)
    //
    //     sh "docker cp ./srcs ${container_name}:/home/jenkins/${project}"
    //     sh "rm -rf srcs"
    //
    //     stage('Formatting') {
    //         def formatting_script = """
    //             cd ${project}
    //             find . \\( -name '*.cpp' -or -name '*.h' -or -name '*.hpp' \\) \
    //                 -exec clangformatdiff.sh {} +
    //         """
    //         sh "docker exec ${container_name} sh -c \"${formatting_script}\""
    //     }
    // } finally {
    //     container.stop()
    // }
}
