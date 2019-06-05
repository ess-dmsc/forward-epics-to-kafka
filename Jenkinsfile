@Library('ecdc-pipeline')
import ecdcpipeline.ContainerBuildNode
import ecdcpipeline.PipelineBuilder

project = "forward-epics-to-kafka"

clangformat_os = "debian9"
test_and_coverage_os = "centos7"
release_os = "centos7-release"
eee_os = "centos7"

epics_dir = "/opt/epics"
epics_profile_file = "/etc/profile.d/ess_epics_env.sh"

// Set number of old builds to keep.
properties([[
    $class: 'BuildDiscarderProperty',
    strategy: [
        $class: 'LogRotator',
        artifactDaysToKeepStr: '',
        artifactNumToKeepStr: '10',
        daysToKeepStr: '',
        numToKeepStr: '10'
    ]
]]);

container_build_nodes = [
  'centos7': ContainerBuildNode.getDefaultContainerBuildNode('centos7'),
  'centos7-release': ContainerBuildNode.getDefaultContainerBuildNode('centos7'),
  'debian9': ContainerBuildNode.getDefaultContainerBuildNode('debian9'),
  'ubuntu1804': ContainerBuildNode.getDefaultContainerBuildNode('ubuntu1804')
]


pipeline_builder = new PipelineBuilder(this, container_build_nodes)
pipeline_builder.activateEmailFailureNotifications()

builders = pipeline_builder.createBuilders { container ->

    pipeline_builder.stage("${container.key}: checkout") {
        dir(pipeline_builder.project) {
            scm_vars = checkout scm
        }
        // Copy source code to container
        container.copyTo(pipeline_builder.project, pipeline_builder.project)
    }  // stage

    pipeline_builder.stage("${container.key}: get dependencies") {
        container.sh """
            mkdir build
            cd build
            conan remote add --insert 0 ess-dmsc-local ${local_conan_server}
        """
    }  // stage
    
    pipeline_builder.stage("${container.key}: configure") {
        if (container.key != release_os) {
            def coverage_on
            if (container.key == test_and_coverage_os) {
                coverage_on = "-DCOV=1"
            } else {
                coverage_on = ""
            }
            
            def configure_epics = ""
            //if (container.key == eee_os) {
            //    // Only use the host machine's EPICS environment on eee_os
            //    configure_epics = ". ${epics_profile_file}"
            //} else {
            //    // A problem is caused by "&& \" if left empty
            //    configure_epics = "true"
            //}

            container.sh """
                cd build
                ${configure_epics}
                cmake -DCMAKE_BUILD_TYPE=Debug ../${pipeline_builder.project} ${coverage_on}
            """
        } else {
            container.sh """
                cd build
                cmake -DCMAKE_SKIP_BUILD_RPATH=ON -DCMAKE_BUILD_TYPE=Release ../${pipeline_builder.project}
            """
        }  // if/else
    }  // stage
    
    pipeline_builder.stage("${container.key}: build") {
        container.sh """
            cd build
            . ./activate_run.sh
            make VERBOSE=1
        """
    }  // stage

    pipeline_builder.stage("${container.key}: test") {
        if (container.key == test_and_coverage_os) {
            // Run tests with coverage.
            def test_output = "TestResults.xml"
            container.sh """
                cd build
                . ./activate_run.sh
                ./tests/tests -- --gtest_output=xml:${test_output}
                make coverage
                lcov --directory . --capture --output-file coverage.info
                lcov --remove coverage.info '*_generated.h' '*/src/date/*' '*/.conan/data/*' '*/usr/*' --output-file coverage.info
                pkill caRepeater || true
            """

            container.copyFrom('build', '.')
            junit "build/${test_output}"

            withCredentials([string(credentialsId: 'forward-epics-to-kafka-codecov-token', variable: 'TOKEN')]) {
                sh "cp ${pipeline_builder.project}/codecov.yml codecov.yml"
                sh "curl -s https://codecov.io/bash | bash -s - -f build/coverage.info -t ${TOKEN} -C ${scm_vars.GIT_COMMIT}"
            }  // withCredentials
        } else {
            // Run tests.
            container.sh """
                cd build
                . ./activate_run.sh
                ./tests/tests
                pkill caRepeater || true
            """
        }  // if/else
    }  // stage

    if (container.key == release_os) {
        pipeline_builder.stage("${container.key}: archive") {
            container.sh """
                cd build
                rm -rf forward-epics-to-kafka; mkdir forward-epics-to-kafka
                mkdir -p forward-epics-to-kafka/bin
                cp ./bin/forward-epics-to-kafka forward-epics-to-kafka/bin/
                cp -r ./lib forward-epics-to-kafka/
                cp -r ./licenses forward-epics-to-kafka/
                tar czf ${archive_output} forward-epics-to-kafka
                # Create file with build information
                touch BUILD_INFO
                echo 'Repository: ${project}/${env.BRANCH_NAME}' >> BUILD_INFO
                echo 'Commit: ${scm_vars.GIT_COMMIT}' >> BUILD_INFO
                echo 'Jenkins build: ${BUILD_NUMBER}' >> BUILD_INFO
            """
            container.copyFrom("build/${archive_output}", '.')
            container.copyFrom("build/BUILD_INFO", '.')
            archiveArtifacts "${archive_output},BUILD_INFO"
        }  // stage
    }  // if

    if (container.key == clangformat_os) {
        pipeline_builder.stage("${container.key}: Formatting") {
            if (!env.CHANGE_ID) {
            // Ignore non-PRs
            return
            }
            try {
                container.sh """
                clang-format -version
                cd ${pipeline_builder.project}
                find . \\\\( -name '*.cpp' -or -name '*.cxx' -or -name '*.h' -or -name '*.hpp' \\\\) \\
                -exec clang-format -i {} +
                git config user.email 'dm-jenkins-integration@esss.se'
                git config user.name 'cow-bot'
                git status -s
                git add -u
                git commit -m 'GO FORMAT YOURSELF'
                """
                withCredentials([
                usernamePassword(
                credentialsId: 'cow-bot-username',
                usernameVariable: 'USERNAME',
                passwordVariable: 'PASSWORD'
                )
                ]) {
                container.sh """
                    cd ${pipeline_builder.project}
                    git push https://${USERNAME}:${PASSWORD}@github.com/ess-dmsc/forward-epics-to-kafka.git HEAD:${CHANGE_BRANCH}
                """
                } // withCredentials
            } catch (e) {
            // Okay to fail as there could be no badly formatted files to commit
            } finally {
                // Clean up
            }
        }
        
        pipeline_builder.stage("${container.key}: Cppcheck") {
        def test_output = "cppcheck.xml"
            container.sh """
            cd ${pipeline_builder.project}
            cppcheck --xml --inline-suppr --enable=all --inconclusive src/ 2> ${test_output}
            """
            container.copyFrom("${pipeline_builder.project}/${test_output}", '.')
            recordIssues sourceCodeEncoding: 'UTF-8', qualityGates: [[threshold: 1, type: 'TOTAL', unstable: true]], tools: [cppCheck(pattern: 'cppcheck.xml', reportEncoding: 'UTF-8')]
        } // stage
    }  // if
}  // create builders

def failure_function(exception_obj, failureMessage) {
    def toEmails = [[$class: 'DevelopersRecipientProvider']]
    emailext body: '${DEFAULT_CONTENT}\n\"' + failureMessage + '\"\n\nCheck console output at $BUILD_URL to view the results.', recipientProviders: toEmails, subject: '${DEFAULT_SUBJECT}'
    throw exception_obj
}

def get_win10_pipeline() {
    return {
        node('windows10') {
        
        // Use custom location to avoid Win32 path length issues
            ws('c:\\jenkins\\') {
                cleanWs()
                dir("${project}") {
                    stage("win10: Checkout") {
                      checkout scm
                    }  // stage

                    stage("win10: Setup") {
                        // "conan remove" is temporary, until all repos have migrated to official flatbuffers package
                        bat """conan remove -f FlatBuffers/*
                        if exist _build rd /q /s _build
                        mkdir _build
                        """
                    } // stage
                    
                    stage("win10: Install") {
                      bat """cd _build
                    conan.exe \
                        install ..\\conan\\conanfile_win32.txt  \
                        --settings build_type=Release \
                        --build=outdated"""
                    }  // stage

                    stage("win10: Build") {
                           bat """cd _build
                        cmake .. -G \"Visual Studio 15 2017 Win64\" -DCMAKE_BUILD_TYPE=Release -DCONAN=MANUAL
                        cmake --build . --config Release
                        """
                    }  // stage
                }  // dir
            }  // ws
        }  // node
    }  // return
}  // def

def get_system_tests_pipeline() {
    return {
        node('system-test') {
            cleanWs()
            dir("${project}") {
                try{
                    stage("System tests: Checkout") {
                        checkout scm
                    }  // stage
                    stage("System tests: Install requirements") {
                        sh """python3.6 -m pip install --user --upgrade pip
                        python3.6 -m pip install --user -r system-tests/requirements.txt
                        """
                    }  // stage
                    stage("System tests: Run") {
                        sh """docker stop \$(docker ps -a -q) && docker rm \$(docker ps -a -q) || true
                                                """
			timeout(time: 30, activity: true){
                            sh """cd system-tests/
                            python3.6 -m pytest -s  --junitxml=./SystemTestsOutput.xml ./
                            """
			}
                    }  // stage
                } finally {
		    stage("System tests: Cleanup") {
                        sh """docker stop \$(docker ps -a -q) && docker rm \$(docker ps -a -q) || true
                        """
                    }  // stage
                    stage("System tests: Archive") {
                        junit "system-tests/SystemTestsOutput.xml"
                        archiveArtifacts "system-tests/logs/*.log"
                    }
                }
            } // dir
        }  // node
    }  // return
}  // def

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

    builders['windows10'] = get_win10_pipeline()

    if ( env.CHANGE_ID ) {
        builders['system tests'] = get_system_tests_pipeline()
    }

    parallel builders
	
    // Delete workspace when build is done
    cleanWs()
}
