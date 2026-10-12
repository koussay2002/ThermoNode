// =====================================================================================
//  Jenkinsfile — modèle à compléter par les étudiants (TP 5 / projet)
//  Chaîne imposée : BUILD -> ANALYSE STATIQUE (SonarQube) -> TESTS -> PACKAGE -> DEPLOIEMENT
//  Chaque "TODO" est une étape à écrire. Les portes qualité doivent être BLOQUANTES et
//  prouvées par une injection de défaut (build rouge) puis un retrait (build vert).
// =====================================================================================
pipeline {
  agent none
  options { timestamps(); timeout(time: 30, unit: 'MINUTES'); disableConcurrentBuilds(abortPrevious: true) }
  environment {
    NEXUS   = 'http://nexus:8081/repository/thermonode-raw'
    VERSION = ''                                   // renseignée au checkout (git describe)
  }
  triggers { pollSCM('H/2 * * * *') }             // TODO (P4) : remplacer par un webhook de la forge

  stages {

    // ---------- 1. SOURCE ------------------------------------------------------------------
    stage('Checkout') {
      agent any
      steps {
        checkout scm
        script { env.VERSION = sh(script: 'git describe --tags --always --dirty', returnStdout: true).trim() }
        stash name: 'src', useDefaultExcludes: false
      }
    }

    // ---------- 2. BUILD -------------------------------------------------------------------
    stage('Build') {
      agent { dockerfile { filename 'ci/Dockerfile.build' } }        // image de build versionnée
      steps {
        unstash 'src'
        sh 'mkdir -p reports && make -C firmware clean build'         // -Wall -Wextra -Werror : porte 0 avertissement
        sh 'make -C firmware cross'                                   // compilation croisée Cortex-M0+ (ENF-03)
        // TODO (G4) : porte budget mémoire -> make -C firmware size-check
      }
    }

    // ---------- 3. ANALYSE STATIQUE : SONARQUBE ---------------------------------------------
    // Les analyseurs (cppcheck/MISRA pour le C, l'analyseur Python natif de Sonar) alimentent
    // UN SEUL tableau de bord et UNE SEULE porte : la Quality Gate SonarQube.
    stage('Static analysis (SonarQube)') {
      agent { dockerfile { filename 'ci/Dockerfile.build' } }
      steps {
        unstash 'src'
        sh 'make -C firmware static'                                  // -> reports/cppcheck.xml
        sh 'python3 tools/cppcheck2sonar.py reports/cppcheck.xml > reports/cppcheck-sonar.json'
        // Les rapports de tests/couverture sont produits au stage suivant ; on lance donc
        // le scanner APRES les tests (voir stage 'Quality gate'). Ici : seulement la collecte.
        stash name: 'static', includes: 'reports/**'
      }
    }

    // ---------- 4. TESTS -------------------------------------------------------------------
    stage('Tests') {
      parallel {
        stage('Unit tests firmware') {
          agent { dockerfile { filename 'ci/Dockerfile.build' } }
          steps { unstash 'src'; sh 'make -C firmware test coverage'; stash name: 'rep-fw', includes: 'reports/*-fw.*' }
        }
        stage('Unit tests gateway') {
          agent { dockerfile { filename 'ci/Dockerfile.build' } }
          steps {
            unstash 'src'
            sh 'cd gateway && pytest tests/unit --junitxml=../reports/junit-gw.xml --cov=gateway --cov-branch --cov-report=xml:../reports/coverage-gw.xml'
            stash name: 'rep-gw', includes: 'reports/*-gw.xml'
          }
        }
      }
      post { always {
        node('') {   // TODO : remplacer par un label d'agent si nécessaire
          unstash 'rep-fw'; unstash 'rep-gw'
          junit testResults: 'reports/junit-*.xml', allowEmptyResults: false
          script { if (currentBuild.result == 'UNSTABLE') { error('Tests en échec : build bloqué') } }   // porte : 0 échec
          // TODO (G4) : porte couverture -> recordCoverage ... qualityGates: [[threshold: 85.0, metric: 'BRANCH', criticality: 'FAILURE']]
          // TODO (G4) : porte traçabilité -> python3 tools/traceability.py docs/requirements.md reports/junit-*.xml --fail-on-uncovered P1
          stash name: 'tests', includes: 'reports/**'
        }
      } }
    }

    stage('Integration tests') {
      agent any
      steps {
        unstash 'src'
        sh 'docker compose -f ci/compose.test.yaml up -d --build --wait'
        sh 'cd gateway && pip3 install --break-system-packages -q -r requirements.txt && MQTT_HOST=localhost pytest tests/integration --junitxml=../reports/junit-int.xml --timeout=180'
      }
      post { always { sh 'docker compose -f ci/compose.test.yaml down -v'; junit testResults: 'reports/junit-int.xml', allowEmptyResults: true } }
    }

    // ---------- 5. PORTE QUALITE SONARQUBE --------------------------------------------------
    stage('Quality gate (SonarQube)') {
      agent { dockerfile { filename 'ci/Dockerfile.build' } }
      steps {
        unstash 'src'; unstash 'static'; unstash 'tests'
        withSonarQubeEnv('sonar') { sh 'sonar-scanner -Dsonar.projectVersion=${VERSION}' }   // envoie code + cppcheck + JUnit + couverture
        timeout(time: 5, unit: 'MINUTES') { waitForQualityGate abortPipeline: true }         // porte : Quality Gate "TVD" Passed
      }
    }

    // ---------- 6. PACKAGE -----------------------------------------------------------------
    stage('Package & archive') {
      when { anyOf { branch 'main'; buildingTag() } }
      agent { dockerfile { filename 'ci/Dockerfile.build' } }
      steps {
        unstash 'src'
        sh '''
          make -C firmware cross && mkdir -p dist
          arm-none-eabi-objcopy -O binary firmware/build/thermonode-core.elf dist/thermonode-fw-${VERSION}.bin
          cd gateway && pip3 install --break-system-packages -q build && python3 -m build --wheel -o ../dist . && cd ..
          cd dist && sha256sum * > SHA256SUMS && printf "version=%s\\ncommit=%s\\nbuild=%s\\n" "${VERSION}" "${GIT_COMMIT}" "${BUILD_NUMBER}" > MANIFEST
        '''
        archiveArtifacts artifacts: 'dist/*', fingerprint: true
        // TODO (P2) : publication dans Nexus (withCredentials + curl --upload-file), dépôt raw en "Disable redeploy"
      }
    }

    // ---------- 7. DEPLOIEMENT -------------------------------------------------------------
    stage('Deploy to staging') {
      when { branch 'main' }
      agent any
      steps {
        unstash 'src'
        sh 'docker build -t localhost:8082/thermonode/gateway:${VERSION} gateway'
        sh 'VERSION=${VERSION} docker compose -f deploy/staging.yaml up -d'
        sh 'cd gateway && GATEWAY_URL=http://localhost:8001 MQTT_HOST=localhost MQTT_PORT=1884 pytest tests/smoke --timeout=60'   // tests de fumée
      }
    }

    stage('Deploy to production (pilot fleet)') {
      when { buildingTag() }
      agent any
      steps {
        input message: "Déployer ${VERSION} sur la flotte pilote ?", ok: 'Déployer', submitter: 'qa-lead'    // décision humaine = livraison continue
        // TODO (P3) : tools/ota_push.py --fleet pilot --version ${VERSION} --from ${NEXUS} ; archiver deploy/log/*.json
      }
    }
  }

  post {
    failure { echo "Pipeline en échec : ${env.BUILD_URL}" }   // TODO : notification (mail / Slack) de l'équipe
  }
}
