/* eslint-disable @typescript-eslint/no-unnecessary-condition */
import { http, HttpResponse } from 'msw';
import type { SettingsRequest } from '../api/Requests';
import type { SysInfoResponse, SettingsResponse } from '../api/Responses';

// TODO Use localStorage to persist settings across reloads

let settings: SettingsResponse = {
    minDepartureMinutes: 0,
    maxDepartureCount: 12,
    currentStation: null,
};

const ENABLE_FAILURES = false;
const FAILURE_RATE = 0.2;

export const handlers = [
    http.get('/api/sysinfo', () => {
        const enableTrace = true;
        const enableRuntime = true;
        const enableCoreId = true;

        const response: SysInfoResponse = {
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
            tasks: enableTrace
                ? [...Array<number>(10)].map((_, index) => ({
                      name: `TaskNumber${index.toString()}`,
                      priority: index,
                      state: Math.floor(Math.random() * 6),
                      stack_high_water_mark: Math.floor(Math.random() * 10000),
                      runtime: enableRuntime ? Math.floor(Math.random() * 100) : null,
                      core_id: enableCoreId ? Math.floor(Math.random() * 2) : null,
                  }))
                : null,
        };

        return HttpResponse.json(response);
    }),

    http.get('/api/settings', () => {
        if (ENABLE_FAILURES && Math.random() < FAILURE_RATE) {
            return new HttpResponse(null, { status: 500 });
        }
        return HttpResponse.json(settings);
    }),

    http.post('/api/settings', async ({ request }) => {
        if (ENABLE_FAILURES && Math.random() < FAILURE_RATE) {
            return new HttpResponse(null, { status: 500 });
        }

        const body = (await request.json()) as SettingsRequest;
        settings = { ...settings, ...body };
        console.log('Updated settings:', JSON.stringify(settings, null, 2));

        return HttpResponse.json({});
    }),
];
