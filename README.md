# cmdalias
Just a small tool to help me (and you but without any guaranty) to create command aliases


# Installing with Docker

If you do not want to install the required global dependencies (bison, flex, g++),
you can compile this utility using docker.

Running `make docker-image` will create the docker image necessary to run cmdalias.

You can then do:

```
source <(docker run -v $HOME/.cmdalias:/root/.cmdalias -it cmdalias cmdalias -i)
```

