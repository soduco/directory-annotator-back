#!/bin/bash
cat docker-swarm-ro-token.txt | docker login -u docker-swarm --password-stdin gitlab-registry.lre.epita.fr/soduco/directory-annotator-back
docker stack deploy --with-registry-auth -c docker-compose-back-standalone-swarm.yml soduco-back
