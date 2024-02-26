# Pulling the docker image
*Same commands on Linux, Windows and MacOS*

FIXME

```
docker login gitlab-registry.lrde.epita.fr
docker pull gitlab-registry.lrde.epita.fr/soduco/directory-annotator-back
```




# Running the server (Linux)

1. Create a `.env` file from one of the samples.
2. Edit the file ``.env`` with the correct paths.
3. **If you want to avoid rebuilding the image**, pre-fetch the image using `docker pull gitlab-registry.lrde.epita.fr/soduco/directory-annotator-back`
4. Run the service

```
$ docker-compose up
backend_1  |  * Serving Flask app "server.app_server" (lazy loading)
backend_1  |  * Environment: development
backend_1  |  * Debug mode: on
backend_1  | INFO:  * Running on http://0.0.0.0:5000/ (Press CTRL+C to quit)
backend_1  | INFO:  * Restarting with stat
backend_1  | WARNING:  * Debugger is active!
backend_1  | INFO:  * Debugger PIN: 179-768-730
...
```

4. Test locally

```
$ curl -X GET "http://localhost:8000/directories/" -H "Authorization:12345678"     
{
  "directories": [
    "Didot_1843a.pdf", 
    "Didot_1847.pdf", 
    "Didot_1845a.pdf", 
    "Didot_1851a.pdf", 
    "Didot_1841a.pdf", 
    "Didot_1844a.pdf", 
    "Didot_1846.pdf", 
    "Didot_1848a.pdf", 
    "Didot_1850a.pdf", 
    "Didot_1842a.pdf", 
    "Didot_1849a.pdf"
  ]
}
```

Note, you may need to **disable SELinux** (e.g. on Fedora):

    ```
    setenforce 0
    ```

# Building locally the image

1. Get the latest C++ module built for [Debian Stable](https://gitlab.lrde.epita.fr/soduco/directory-annotator-back/-/jobs/artifacts/master/download?job=build+debian+stable)

2. Unzip ``artifacts.zip`` in the root project directory

```
$ ls build
soduco-py37-0.1.1-Linux.tar.gz
```

3. In the ``docker`` folder, run ``docker-compose build``

