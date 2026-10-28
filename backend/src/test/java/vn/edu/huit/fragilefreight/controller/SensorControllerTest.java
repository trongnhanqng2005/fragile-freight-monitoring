package vn.edu.huit.fragilefreight.controller;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.Mockito.when;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.WebMvcTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.http.MediaType;
import org.springframework.test.web.servlet.MockMvc;
import org.springframework.test.web.servlet.request.MockMvcRequestBuilders;
import org.springframework.test.web.servlet.result.MockMvcResultMatchers;
import vn.edu.huit.fragilefreight.model.SensorState;
import vn.edu.huit.fragilefreight.service.SensorStateService;

@WebMvcTest(SensorController.class)
class SensorControllerTest {

    @Autowired
    private MockMvc mockMvc;

    @MockBean
    private SensorStateService sensorStateService;

    @Test
    void getLatestSensorState_shouldReturnLatestStateWhenPresent() throws Exception {
        SensorState sensorState = new SensorState("esp32-01", 0.02, -0.01, 1.00, 1.00, "SAFE");
        when(sensorStateService.getLatestState()).thenReturn(java.util.Optional.of(sensorState));

        mockMvc.perform(MockMvcRequestBuilders.get("/api/sensor/latest")
                .accept(MediaType.APPLICATION_JSON))
            .andExpect(MockMvcResultMatchers.status().isOk())
            .andExpect(MockMvcResultMatchers.jsonPath("$.deviceId").value("esp32-01"))
            .andExpect(MockMvcResultMatchers.jsonPath("$.status").value("SAFE"));
    }
}
