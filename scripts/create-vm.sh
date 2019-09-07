#!/usr/bin/env bash

# GCP Compute VM
gcloud beta compute --project=foijord-project instances create instance-1 \
--zone=europe-west1-d \
--machine-type=n1-standard-4 \
--subnet=default \
--network-tier=PREMIUM \
--metadata=ssh-keys=foijord:ssh-rsa\ AAAAB3NzaC1yc2EAAAADAQABAAACAQDMv9ncQcD0RZoP5Pl1j3loq/6Bfd8xCzFvmU12sURMBSgQw871dknfjf4RfWebN1tXRkQ5doG\+AJG1pEBehYMFhxKSIu5bsw\+JzJIrDKJ1arWGMsC4l9o6Xk\+IGV62DVWGcuGMONgiPAzJBMzJ4qytY6zGYzOOEUoisoTna2cKIk\+of9teVz13jKyZ4dpXoYPNocDDMy0ttSxolufKOzpDbveTHr2oHbuYsoytA4tNdBV8F998TFp\+0lepKo43RW1NOC4qjNhpbbp2GWJNnMDlS4/IfyJI5mEYlmj4yvI0570ybUJUn1co\+po7I4zaAjIf8F\+GkqhK7//Aenly78YyuuTO2iVHqqW6WXth41q8RIy0VfPQR0jp2o3bhY1bHJNLTQraeSTNRfl9AF5LNlkw7BHErSmmE\+dxpwjPARZqUaRFc3NuZD5UyuZwOgCgfsqS0JxRyd8h0\+R\+gH9WURuGdq\+ES\+V6EqnxhJit9Xsf8IIClFJJps2ej/EGkB3jCz6yAz9vuP2XQHf9aw6Ii/lZIPme17aMSL2xmmY1wZvDCpUiu99qhBQ24/x9gKJ3zr7xORXoT99TwKt7l4LjQr8Acqb43MCzr7W2p2jsthHSgYtMsOkXm\+rI7\+TJVBhuuUAGmj/cqhzzAm0piFg\+T8iANikKFgY/ysD\+pGmAIWTiOQ==\ foijord@DESKTOP-1MG0B62 \
--maintenance-policy=TERMINATE \
--service-account=563567856031-compute@developer.gserviceaccount.com \
--scopes=https://www.googleapis.com/auth/devstorage.read_only,https://www.googleapis.com/auth/logging.write,https://www.googleapis.com/auth/monitoring.write,https://www.googleapis.com/auth/servicecontrol,https://www.googleapis.com/auth/service.management.readonly,https://www.googleapis.com/auth/trace.append \
--accelerator=type=nvidia-tesla-k80,count=1 \
--image=ubuntu-1804-bionic-v20190813a \
--image-project=ubuntu-os-cloud \
--boot-disk-size=256GB \
--boot-disk-type=pd-ssd \
--boot-disk-device-name=instance-1 \
--reservation-affinity=any
