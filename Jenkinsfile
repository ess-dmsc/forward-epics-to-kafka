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

        stage('Check Coverage') {
            sh "/usr/bin/pip install cpp-coveralls && ls /usr/local/bin/ && /usr/local/bin/cpp-coveralls -t 'xtf16Nv5y5SdMjUtFQpuBLaYpizESdGRU' -e src/tests/"
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
