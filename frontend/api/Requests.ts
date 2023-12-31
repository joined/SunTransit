import { LineProductType, ParsedStation } from '../Types';

export interface LocationsQueryRequestQuerySchema {
    query: string;
    fuzzy?: boolean; // default: true
    results?: number; // default: unknown
    stops?: boolean; // show stops/stations? default: true
    addresses: boolean; // show addresses? default: true
    poi: boolean; // show POIs? default: true
    linesOfStops?: boolean; // parse & return lines of stops? default: false
    language: 'en';
    [key: string]: string | boolean | number | undefined;
}

export interface CurrentStationPostRequestSchema extends ParsedStation {
    enabledProducts: Array<LineProductType>;
}
