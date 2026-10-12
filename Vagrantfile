# Vagrantfile ThermoNode — VM Ubuntu 24.04 outillée pour le module EET3S1.3-TVD
# Usage : vagrant up ; vagrant ssh ; make -C firmware test
#
# Organisation des dossiers :
#   /vagrant        dossier du PC partagé par VirtualBox : sert UNIQUEMENT au transfert
#                   (lent, et les binaires compilés n'y sont pas exécutables sous Windows)
#   /home/vagrant/tn  copie de travail sur le disque de la VM : c'est ICI que l'on travaille
#   tn-pull         recopie PC  -> VM (après une modification faite sous Windows)
#   tn-push         recopie VM  -> PC (pour ouvrir les rapports HTML depuis Windows)
Vagrant.configure("2") do |config|
  config.vm.box = "bento/ubuntu-24.04"
  config.vm.hostname = "thermonode"

  config.vm.provider "virtualbox" do |vb|
    vb.name   = "thermonode-tvd"
    vb.memory = 4096
    vb.cpus   = 2
    vb.customize ["modifyvm", :id, "--natdnshostresolver1", "on"]   # DNS via l'hôte
  end

  # Dossier partagé VirtualBox (transfert seulement, voir en-tête)
  config.vm.synced_folder ".", "/vagrant"

  config.vm.network "forwarded_port", guest: 8080, host: 8080   # Jenkins
  config.vm.network "forwarded_port", guest: 9000, host: 9000   # SonarQube
  config.vm.network "forwarded_port", guest: 8081, host: 8081   # Nexus
  config.vm.network "forwarded_port", guest: 1883, host: 1883   # broker MQTT
  config.vm.network "forwarded_port", guest: 8000, host: 8000   # passerelle
  config.vm.network "forwarded_port", guest: 8001, host: 8001   # staging

  config.vm.provision "shell", inline: <<-SHELL
    export DEBIAN_FRONTEND=noninteractive
    apt-get update
    apt-get install -y build-essential gcc-arm-none-eabi binutils-arm-none-eabi cppcheck git make rsync \
        python3 python3-pip python3-venv mosquitto-clients unzip curl docker.io docker-compose-v2
    usermod -aG docker vagrant
    sysctl -w vm.max_map_count=262144
    grep -q max_map_count /etc/sysctl.conf || echo "vm.max_map_count=262144" >> /etc/sysctl.conf

    # Deux commandes de synchronisation entre le PC (/vagrant) et la copie de travail (~/tn)
    cat > /usr/local/bin/tn-pull <<'EOF'
#!/bin/bash
# PC -> VM : recopie le projet depuis le dossier partagé vers la copie de travail
rsync -a --exclude build/ --exclude reports/ --exclude venv/ --exclude .venv/ --exclude .vagrant/ /vagrant/ /home/vagrant/tn/
echo "tn-pull : /vagrant -> ~/tn terminé"
EOF
    cat > /usr/local/bin/tn-push <<'EOF'
#!/bin/bash
# VM -> PC : recopie la copie de travail (dont reports/) vers le dossier partagé
rsync -a --exclude build/ --exclude venv/ --exclude .venv/ --exclude .vagrant/ /home/vagrant/tn/ /vagrant/
echo "tn-push : ~/tn -> /vagrant terminé (rapports visibles depuis Windows)"
EOF
    chmod +x /usr/local/bin/tn-pull /usr/local/bin/tn-push

    # Copie de travail initiale sur le disque de la VM
    sudo -u vagrant mkdir -p /home/vagrant/tn
    sudo -u vagrant /usr/local/bin/tn-pull

    # Environnement Python
    sudo -u vagrant python3 -m venv /home/vagrant/venv
    sudo -u vagrant /home/vagrant/venv/bin/pip install -q gcovr -r /home/vagrant/tn/gateway/requirements.txt
    grep -q "venv/bin/activate" /home/vagrant/.bashrc || echo "source ~/venv/bin/activate && cd ~/tn" >> /home/vagrant/.bashrc

    for img in eclipse-mosquitto:2 python:3.12-slim gcc:13 debian:bookworm-slim; do docker pull -q $img || true; done
  SHELL
end
