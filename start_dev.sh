#!/usr/bin/env sh
docker compose run --rm builder || docker-compose run --rm builder || echo "Failed: Please start the docker service"
