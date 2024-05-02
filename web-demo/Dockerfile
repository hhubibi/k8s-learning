FROM python:3

WORKDIR /app

ADD . /app

RUN pip install -r requirements.txt

EXPOSE 8080

ENV NAME Test

CMD ["python3", "app.py"]