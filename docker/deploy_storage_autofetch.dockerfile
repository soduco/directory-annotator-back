FROM python:3.7-slim-buster
# Python deps
# COPY . /tmp/directory_annotator_storage

# We use a readonly token here
RUN pip install -q --no-cache-dir \
    --use-pep517 \
    directory-annotator-storage \
    gunicorn \
    --extra-index-url https://autodeploy:Wc1eLFEkgn_kyKft5k9L@gitlab.lre.epita.fr/api/v4/projects/799/packages/pypi/simple


ENV LC_ALL=C
WORKDIR /app
CMD gunicorn -t 500 --access-logfile - --bind 0.0.0.0:8000 --proxy-allow-from='*' 'directory_annotator_storage:create_app()'

