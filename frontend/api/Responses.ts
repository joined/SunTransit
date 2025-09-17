import { StationWithProducts } from '../Types';

export interface SettingsResponse {
    minDepartureMinutes: number;
    maxDepartureCount: number;
    currentStation: StationWithProducts | null;
}

export interface SysInfoAppStateResponse {
    time: number | null;
    mdns_hostname: string;
}

export interface SysInfoSoftwareResponse {
    app_version: string;
    idf_version: string;
    project_name: string;
    compile_time: string;
    compile_date: string;
}

export interface SysInfoHardwareResponse {
    mac_address: string;
    chip_model: number;
}

export interface SysInfoMemoryResponse {
    free_heap: number;
    minimum_free_heap: number;
}

export interface SysInfoTaskResponse {
    name: string;
    priority: number;
    state: number;
    stack_high_water_mark: number;
    runtime: number | null;
    core_id: number | null;
}

export interface SysInfoResponse {
    app_state: SysInfoAppStateResponse;
    software: SysInfoSoftwareResponse;
    hardware: SysInfoHardwareResponse;
    memory: SysInfoMemoryResponse;
    tasks: Array<SysInfoTaskResponse> | null;
}
