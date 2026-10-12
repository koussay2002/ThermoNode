FROM python:3.12-slim
RUN useradd -m gw && mkdir -p /data && chown gw:gw /data
WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir paho-mqtt==2.1.0
COPY gateway ./gateway
USER gw
ENV DB_PATH=/data/gw.sqlite HTTP_PORT=8000
EXPOSE 8000
CMD ["python", "-m", "gateway"]
