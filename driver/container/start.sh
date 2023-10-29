#!/bin/bash
set -euo pipefail

IMAGE_NAME="surgefuzz-driver"
TAG="latest"

if ! command -v docker &> /dev/null
then
    echo "Error: Docker is not installed. Please install Docker and try again."
    exit 1
fi

if ! docker info &> /dev/null
then
    echo "Error: Docker is not running. Please start Docker and try again."
    exit 1
fi

echo "Building Docker image..."
docker build -t ${IMAGE_NAME}:${TAG} \
             --build-arg USER_UID=$(id -u) \
             --build-arg USER_GID=$(id -g) \
             -f Dockerfile ..

echo "Docker image ${IMAGE_NAME}:${TAG} has been successfully built."

echo "Running Docker container..."
docker run --rm -it \
           --entrypoint=bash \
           ${IMAGE_NAME}:${TAG}

echo "Docker container has been successfully run."
