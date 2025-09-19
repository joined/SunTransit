/* eslint-disable @typescript-eslint/no-unnecessary-condition */
import { http, HttpResponse } from 'msw';
import type { SettingsRequest } from '../api/Requests';
import type { SysInfoResponse, SettingsResponse } from '../api/Responses';
import type { LineProductType } from '../Types';

const MOCK_STORAGE_KEY = 'msw_mock_settings';

function getDefaultSettings(): SettingsResponse {
    return {
        minDepartureMinutes: 0,
        maxDepartureCount: 12,
        showCancelledDepartures: true,
        currentStation: null,
    };
}

function loadSettingsFromStorage(): SettingsResponse {
    try {
        const stored = localStorage.getItem(MOCK_STORAGE_KEY);
        if (stored) {
            return JSON.parse(stored) as SettingsResponse;
        }
    } catch (error) {
        console.warn('Failed to load mock settings from localStorage:', error);
    }

    return getDefaultSettings();
}

function saveSettingsToStorage(newSettings: SettingsResponse): void {
    try {
        localStorage.setItem(MOCK_STORAGE_KEY, JSON.stringify(newSettings));
    } catch (error) {
        console.warn('Failed to save mock settings to localStorage:', error);
    }
}

export function clearMockStorage(): void {
    try {
        localStorage.removeItem(MOCK_STORAGE_KEY);
        console.log('Mock storage cleared, reset to defaults');
    } catch (error) {
        console.warn('Failed to clear mock storage:', error);
    }
}

const buildMockBvgUrl = (station: SettingsResponse['currentStation'], maxResults: number): string | null => {
    if (!station) return null;

    const products: Array<LineProductType> = ['suburban', 'subway', 'tram', 'bus', 'ferry', 'express', 'regional'];
    const productParams = Object.fromEntries(
        products.map((product) => [product, station.enabledProducts.includes(product).toString()])
    );

    const params = new URLSearchParams({
        results: maxResults.toString(),
        pretty: 'false',
        remarks: 'false',
        duration: '60',
        ...productParams,
    });

    return `https://v6.bvg.transport.rest/stops/${station.id}/departures?${params.toString()}`;
};

const ENABLE_FAILURES = false;
const FAILURE_RATE = 0.2;

export const handlers = [
    http.get('/api/sysinfo', () => {
        const enableTrace = true;
        const enableRuntime = true;
        const enableCoreId = true;
        const { currentStation, maxDepartureCount } = loadSettingsFromStorage();

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
            debug: {
                bvg_api_url: buildMockBvgUrl(currentStation, maxDepartureCount),
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
        return HttpResponse.json(loadSettingsFromStorage());
    }),

    http.post('/api/settings', async ({ request }) => {
        if (ENABLE_FAILURES && Math.random() < FAILURE_RATE) {
            return new HttpResponse(null, { status: 500 });
        }

        const body = (await request.json()) as SettingsRequest;
        const currentSettings = loadSettingsFromStorage();
        const updatedSettings = { ...currentSettings, ...body };
        saveSettingsToStorage(updatedSettings);
        console.log('Updated settings:', JSON.stringify(updatedSettings, null, 2));

        return HttpResponse.json({});
    }),
];
