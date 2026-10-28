package vn.edu.huit.fragilefreight.mqtt;

import java.nio.charset.StandardCharsets;
import java.util.UUID;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import vn.edu.huit.fragilefreight.model.SensorState;
import vn.edu.huit.fragilefreight.service.SensorStateService;

@Configuration
public class MqttSubscriberConfig {

    private static final Logger LOGGER = LoggerFactory.getLogger(MqttSubscriberConfig.class);

    @Value("${mqtt.broker-url:tcp://localhost:1883}")
    private String brokerUrl;

    @Value("${mqtt.client-id:fragile-freight-backend}")
    private String clientId;

    @Bean(destroyMethod = "disconnect")
    @ConditionalOnProperty(name = "mqtt.enabled", havingValue = "true", matchIfMissing = true)
    public MqttClient mqttClient(SensorStateService sensorStateService) {
        String uniqueClientId = clientId + "-" + UUID.randomUUID();
        MqttClient mqttClient;

        try {
            mqttClient = new MqttClient(brokerUrl, uniqueClientId);
            MqttConnectOptions options = new MqttConnectOptions();
            options.setAutomaticReconnect(true);
            options.setCleanSession(true);
            options.setConnectionTimeout(10);

            mqttClient.setCallback(new MqttCallback() {
                @Override
                public void connectionLost(Throwable cause) {
                    LOGGER.warn("MQTT connection lost. Attempting reconnect ...", cause);
                }

                @Override
                public void messageArrived(String topic, MqttMessage message) {
                    String payload = new String(message.getPayload(), StandardCharsets.UTF_8);
                    LOGGER.info("MQTT message received on {}: {}", topic, payload);

                    try {
                        if (topic.endsWith("/telemetry")) {
                            SensorState sensorState = MqttMessageParser.parseTelemetry(payload);
                            sensorStateService.updateTelemetry(sensorState);
                        } else if (topic.endsWith("/alert")) {
                            SensorState sensorState = MqttMessageParser.parseAlert(payload);
                            sensorStateService.updateAlert(sensorState.getDeviceId(), sensorState.getGForce());
                        }
                    } catch (IllegalArgumentException ex) {
                        LOGGER.warn("Ignoring invalid MQTT message on {}: {}", topic, payload, ex);
                    }
                }

                @Override
                public void deliveryComplete(IMqttDeliveryToken token) {
                    // No-op as the backend acts as a consumer only.
                }
            });

            mqttClient.connect(options);
            mqttClient.subscribe("fragile-freight/esp32-01/telemetry");
            mqttClient.subscribe("fragile-freight/esp32-01/alert");
            LOGGER.info("Connected to MQTT broker at {} and subscribed to contract topics", brokerUrl);
            return mqttClient;
        } catch (MqttException ex) {
            LOGGER.warn("MQTT broker not available at {}. Backend will continue without MQTT until the broker is reachable.", brokerUrl, ex);
            return null;
        }
    }
}
