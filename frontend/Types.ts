import type React from 'react';

export interface RouteConfig {
    path: string;
    name: string;
    drawerIcon: React.ReactElement;
}

export type LineProductType = 'suburban' | 'subway' | 'tram' | 'bus' | 'ferry' | 'express' | 'regional';

export interface ParsedStation {
    id: string;
    name: string;
    linesByProduct: Partial<Record<LineProductType, Array<{ id: string; name: string }>>>;
}

export interface StationWithProducts extends ParsedStation {
    enabledProducts: Array<LineProductType>;
}
