#! /usr/bin/bash
# Initialize the following variables according to the names specified by you for the azure cluster
ACRNAME=acrindrajeet
RESOURCEGROUPNAME=rg-indrajeet
AKSCLUSTERNAME=aks-indrajeet

#Install the Kubernetes CLI
az aks install-cli

#Connect to cluster using kubectl
az aks get-credentials --resource-group $RESOURCEGROUPNAME --name $AKSCLUSTERNAME

# Build the docker image with the tag containing the acr login server address

docker build -t $ACRNAME.azurecr.io/mh:latest .

# Login the the azure container registry  server.

az acr login --name $ACRNAME

# Push the docker image into the azure container registry

docker push $ACRNAME.azurecr.io/mh:latest

#Create deployments using the manifest file

kubectl create -f motion-highlighter-manifest.yaml
