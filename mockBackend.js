const express = require('express');
const app = express();
const port = 3000;

let settings = {
    minDepartureMinutes: 0,
    currentStation: null,
};
const ENABLE_FAILURES = false;
const FAILURE_RATE = 0.2;

app.use(express.json());

app.get('/sysinfo', (req, res) => {
    const enableTrace = true;
    const enableRuntime = true;
    const enableCoreId = true;
    res.send(
        JSON.stringify({
            app_state: {
                time: Date.now(),
                mdns_hostname: 'suntransit-fake.local',
            },
            software: {
                app_version: 'Mock app version',
                idf_version: 'Mock IDF version',
                project_name: 'Mock project name',
                compile_time: 'Mock compile time',
                compile_date: 'Mock compile date',
            },
            hardware: {
                mac_address: '1A:2B:3C:4D:5E:6F',
                chip_model: 1,
            },
            memory: {
                free_heap: 123456,
                minimum_free_heap: 123456,
            },
            ...(enableTrace
                ? {
                      tasks: [...Array(10)].map((_, index) => ({
                          name: `TaskNumber${index}`,
                          priority: index,
                          state: Math.floor(Math.random() * 6),
                          stack_high_water_mark: Math.floor(Math.random() * 10000),
                          runtime: enableRuntime ? Math.floor(Math.random() * 100) : null,
                          core_id: enableCoreId ? Math.floor(Math.random() * 2) : null,
                      })),
                  }
                : null),
        })
    );
});

app.get('/settings', (req, res) => {
    if (ENABLE_FAILURES && Math.random() < FAILURE_RATE) {
        res.status(500).send();
    } else {
        res.send(JSON.stringify(settings));
    }
});

app.post('/settings', (req, res) => {
    if (ENABLE_FAILURES && Math.random() < FAILURE_RATE) {
        res.status(500).send();
    } else {
        settings = { ...settings, ...req.body };
        console.log('Updated settings:', JSON.stringify(settings, null, 2));
        res.send({});
    }
});

app.listen(port, () => {
    console.log(`Mock backend server listening on port ${port}`);
});
