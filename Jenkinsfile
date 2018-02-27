project = "forward-epics-to-kafka"
clangformat_os = "fedora25"
test_and_coverage_os = "centos7-gcc6"
archive_os = "centos7-gcc6"

epics_dir = "/opt/epics"
epics_profile_file = "/etc/profile.d/ess_epics_env.sh"

images = [
        'centos7-gcc6': [
                'name': 'essdmscdm/centos7-gcc6-build-node:2.1.0',
                'sh'  : '/usr/bin/scl enable rh-python35 devtoolset-6 -- /bin/bash'
        ],
        'fedora25'    : [
                'name': 'essdmscdm/fedora25-build-node:1.0.0',
                'sh'  : 'sh'
        ],
        'ubuntu1604'  : [
                'name': 'essdmscdm/ubuntu16.04-build-node:2.1.0',
                'sh'  : 'sh'
        ],
        'ubuntu1710': [
                'name': 'essdmscdm/ubuntu17.10-build-node:2.0.0',
                'sh': 'sh'
        ]
]

base_container_name = "${project}-${env.BRANCH_NAME}-${env.BUILD_NUMBER}"

def Object container_name(image_key) {
    return "${base_container_name}-${image_key}"
}

def failure_function(exception_obj, failureMessage) {
    def toEmails = [[$class: 'DevelopersRecipientProvider']]
    emailext body: '${DEFAULT_CONTENT}\n\"' + failureMessage + '\"\n\nCheck console output at $BUILD_URL to view the results.', recipientProviders: toEmails, subject: '${DEFAULT_SUBJECT}'
    slackSend color: 'danger', message: "${project}: " + failureMessage
    throw exception_obj
}

def Object create_container(image_key) {
    def image = docker.image(images[image_key]['name'])
    def container = image.run("\
        --name ${container_name(image_key)} \
        --tty \
        --network=host \
        --env http_proxy=${env.http_proxy} \
        --env https_proxy=${env.https_proxy} \
        --env local_conan_server=${env.local_conan_server} \
        --mount=type=bind,src=${epics_dir},dst=${epics_dir},readonly \
        --mount=type=bind,src=${epics_profile_file},dst=${epics_profile_file},readonly \
        ")
}

def docker_copy_code(image_key) {
    def custom_sh = images[image_key]['sh']
    sh "docker cp ${project} ${container_name(image_key)}:/home/jenkins/${project}"
    sh """docker exec --user root ${container_name(image_key)} ${custom_sh} -c \"
                        chown -R jenkins.jenkins /home/jenkins/${project}
                        \""""
}

def docker_dependencies(image_key) {
    try {
        def custom_sh = images[image_key]['sh']
        def conan_remote = "ess-dmsc-local"
        def dependencies_script = """
                        mkdir build
                        cd build
                        conan remote add \
                            --insert 0 \
                            ${conan_remote} ${local_conan_server}
                        cat ../${project}/CMakeLists.txt
                        conan install --build=outdated ../${project}/conan/conanfile.txt
                    """
        sh "docker exec ${container_name(image_key)} ${custom_sh} -c \"${dependencies_script}\""

        def checkout_script = """
                        git clone -b master https://github.com/ess-dmsc/streaming-data-types.git
                    """
        sh "docker exec ${container_name(image_key)} ${custom_sh} -c \"${checkout_script}\""
    } catch (e) {
        failure_function(e, "Get dependencies for (${container_name(image_key)}) failed")
    }
}

def docker_cmake(image_key) {
    try {
        def custom_sh = images[image_key]['sh']
        def coverage_on = ""
        if (image_key == test_and_coverage_os) {
            coverage_on = "-DCOV=1"
        }
        def configure_script = """
                        cd build
                        . ./activate_run.sh
                        . ${epics_profile_file}
                        cmake ../${project} -DREQUIRE_GTEST=ON ${coverage_on}
                    """
        sh "docker exec ${container_name(image_key)} ${custom_sh} -c \"${configure_script}\""
    } catch (e) {
        failure_function(e, "CMake step for (${container_name(image_key)}) failed")
    }
}

def docker_build(image_key) {
    try {
        def custom_sh = images[image_key]['sh']
        def build_script = """
                      cd build
                      . ./activate_run.sh
                      make VERBOSE=1
                  """
        sh "docker exec ${container_name(image_key)} ${custom_sh} -c \"${build_script}\""
    } catch (e) {
        failure_function(e, "Build step for (${container_name(image_key)}) failed")
    }
}

def docker_test(image_key) {
    try {
        def custom_sh = images[image_key]['sh']
        def test_script = """
                        cd build
                        . ./activate_run.sh
                        ./tests/tests
                    """
        sh "docker exec ${container_name(image_key)} ${custom_sh} -c \"${test_script}\""
    } catch (e) {
        failure_function(e, "Test step for (${container_name(image_key)}) failed")
    }
}

def docker_formatting(image_key) {
    try {
        def custom_sh = images[image_key]['sh']
        def script = """
                    clang-format -version
                    cd ${project}
                    find . \\\\( -name '*.cpp' -or -name '*.cxx' -or -name '*.h' -or -name '*.hpp' \\\\) \\
                        -exec clangformatdiff.sh {} +
                  """
        sh "docker exec ${container_name(image_key)} ${custom_sh} -c \"${script}\""
    } catch (e) {
        failure_function(e, "Check formatting step for (${container_name(image_key)}) failed")
    }
}

def docker_archive(image_key) {
    try {
        def custom_sh = images[image_key]['sh']
        def archive_output = "forward-epics-to-kafka.tar.gz"
        def archive_script = """
                    tar czf ${archive_output} build
                """
        sh "docker exec ${container_name(image_key)} ${custom_sh} -c \"${archive_script}\""
        sh "docker cp ${container_name(image_key)}:/home/jenkins/${archive_output} ./"
        archiveArtifacts "${archive_output}"
    } catch (e) {
        failure_function(e, "Test step for (${container_name(image_key)}) failed")
    }
}


def get_pipeline(image_key) {
    return {
        stage("${image_key}") {

            try {
                create_container(image_key)

                docker_copy_code(image_key)
                docker_dependencies(image_key)
                docker_cmake(image_key)

                if (image_key == clangformat_os) {
                    docker_formatting(image_key)
                } else {
                    docker_build(image_key)
                    docker_test(image_key)
                }

                if (image_key == archive_os) {
                    docker_archive(image_key)
                }

            } catch (e) {
                failure_function(e, "Unknown build failure for ${image_key}")
            } finally {
                sh "docker stop ${container_name(image_key)}"
                sh "docker rm -f ${container_name(image_key)}"
            }
        }
    }
}

node('docker && eee') {
    cleanWs()

    stage('Checkout') {
        dir("${project}") {
            try {
                scm_vars = checkout scm
            } catch (e) {
                failure_function(e, 'Checkout failed')
            }
        }
    }

    def builders = [:]
    for (x in images.keySet()) {
        def image_key = x
        builders[image_key] = get_pipeline(image_key)
    }

    parallel builders

    // Delete workspace when build is done
    cleanWs()
}
