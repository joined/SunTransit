import CloseIcon from '@mui/icons-material/Close';
import {
    debounce,
    AutocompleteRenderOptionState,
    Grid,
    Box,
    useMediaQuery,
    Dialog,
    DialogTitle,
    IconButton,
    Stack,
    Autocomplete,
    TextField,
    Button,
    useTheme,
    css,
} from '@mui/material';
import match from 'autosuggest-highlight/match';
import parse from 'autosuggest-highlight/parse';
import axios from 'axios';
import { useState, useMemo, useEffect, HTMLAttributes } from 'react';
import * as R from 'remeda';
import { LocationsQueryRequestQuerySchema } from 'frontend/api/Requests';
import LineIcon from 'frontend/components/LineIcon';
import { ParsedStation, LineProductType, Settings } from 'frontend/Types';

export interface StationChangeDialogProps {
    currentStationId: string | null;
    open: boolean;
    isMutating: boolean;
    saveNewCurrentStation: (newCurrentStation: NonNullable<Settings['currentStation']>, onSuccess: () => void) => void;
    onClose: () => void;
}

interface LocationsQueryLineResponse {
    id: string;
    name: string;
    product: LineProductType;
}

interface LocationsQueryResponseItem {
    id: string;
    name: string;
    lines: Array<LocationsQueryLineResponse>;
}

type LocationsQueryGetResponse = Array<LocationsQueryResponseItem>;

const getLocationsQueryURL = (query: string) => {
    const url = new URL('https://v6.bvg.transport.rest/locations');

    const queryParams: LocationsQueryRequestQuerySchema = {
        query,
        fuzzy: true,
        results: 10,
        stops: true,
        addresses: false,
        poi: false,
        linesOfStops: true,
        language: 'en',
    };

    Object.keys(queryParams).forEach((key) => {
        const value = queryParams[key];
        if (value !== undefined) {
            url.searchParams.append(key, value.toString());
        }
    });

    return url.href;
};

const parseStationItem = ({ id, name, lines }: LocationsQueryResponseItem): ParsedStation => {
    const linesByProduct = R.pipe(
        lines,
        // Seems like some lines don't have an ID, filter them out
        R.filter((line) => !!line.id),
        // Seems like some lines are reported multiple times, the first one looks like the right one
        R.uniqueBy((line) => line.id),
        R.map((line) => ({
            name: line.name,
            // Regional trains are sometimes reported as buses
            product: line.id.startsWith('r') ? 'regional' : line.product,
        })),
        R.groupBy((line) => line.product),
        R.mapValues((lines) => R.map(lines, (line) => line.name))
    );

    return {
        id,
        name,
        linesByProduct,
    };
};

export default function StationChangeDialog({
    currentStationId,
    open,
    isMutating,
    saveNewCurrentStation,
    onClose,
}: StationChangeDialogProps) {
    const [options, setOptions] = useState<Array<ParsedStation>>([]);
    const [selectedOption, setSelectedOption] = useState<ParsedStation | null>(null);
    const [inputValue, setInputValue] = useState('');

    const searchLocations = (query: string) => {
        axios
            .get<LocationsQueryGetResponse>(getLocationsQueryURL(query))
            .then((response) => {
                setOptions(response.data.map(parseStationItem));
            })
            .catch((error: unknown) => {
                console.log('Error fetching locations', error);
            });
    };

    const searchLocationsDebounced = useMemo(() => debounce(searchLocations, 600), []);

    useEffect(() => {
        if (inputValue) {
            searchLocationsDebounced(inputValue);
        }
    }, [searchLocationsDebounced, inputValue, selectedOption]);

    const handleClose = () => {
        onClose();
    };

    const renderOption = (
        props: HTMLAttributes<HTMLLIElement> & { key: unknown },
        { name: stationName, linesByProduct }: ParsedStation,
        state: AutocompleteRenderOptionState
    ) => {
        const matches = match(stationName, state.inputValue, { insideWords: true });
        const parts = parse(stationName, matches);

        // TODO Fix when upgrading material UI? This is sketchy
        // https://github.com/mui/material-ui/issues/39833
        // eslint-disable-next-line react/prop-types
        const { key, ...rest } = props;

        return (
            <li {...rest} key={key as React.Key | undefined | null}>
                <Grid container alignItems="center">
                    <Grid item css={css`width: 'calc(100% - 44px)', word-wrap: 'break-word'`}>
                        {parts.map((part, index) => (
                            <Box
                                key={index}
                                component="span"
                                css={css`
                                    font-weight: ${part.highlight ? 'bold' : 'regular'};
                                `}>
                                {part.text}
                            </Box>
                        ))}
                    </Grid>
                    <Grid item container columnGap={0.5} rowGap={0.5}>
                        {Object.entries(linesByProduct).map(([product, lineNames]) =>
                            lineNames.map((lineName) => (
                                <Grid
                                    item
                                    key={lineName}
                                    css={css`
                                        width: 40px;
                                        height: 15px;
                                    `}>
                                    <LineIcon name={lineName} type={product as LineProductType} />
                                </Grid>
                            ))
                        )}
                    </Grid>
                </Grid>
            </li>
        );
    };

    const isSubmissionDisabled = selectedOption === null || selectedOption.id === currentStationId || isMutating;

    const theme = useTheme();
    const fullScreen = useMediaQuery(theme.breakpoints.down('sm'));

    return (
        <Dialog fullScreen={fullScreen} onClose={handleClose} open={open} PaperProps={{ sx: { overflowY: 'visible' } }}>
            <DialogTitle>Select new station</DialogTitle>
            <IconButton
                aria-label="close"
                onClick={handleClose}
                sx={{
                    position: 'absolute',
                    right: 8,
                    top: 8,
                    color: (theme) => theme.palette.grey[500],
                }}>
                <CloseIcon />
            </IconButton>
            <Stack
                component="form"
                onSubmit={(event) => {
                    event.preventDefault();
                    const newCurrentStation = {
                        /* eslint-disable @typescript-eslint/no-non-null-assertion -- Submission is disabled if the selected option is null */
                        ...selectedOption!,
                        enabledProducts: Object.keys(selectedOption!.linesByProduct) as Array<LineProductType>,
                        /* eslint-enable @typescript-eslint/no-non-null-assertion */
                    };
                    saveNewCurrentStation(newCurrentStation, () => {
                        setSelectedOption(null);
                    });
                }}
                direction={{ xs: 'column', sm: 'row' }}
                padding={2}
                spacing={2}>
                <Autocomplete<ParsedStation>
                    autoComplete
                    includeInputInList
                    filterSelectedOptions
                    noOptionsText="No stations."
                    isOptionEqualToValue={(option, value) => option.id === value.id}
                    options={options}
                    filterOptions={(x) => x}
                    inputValue={inputValue}
                    onInputChange={(event, newInputValue) => {
                        setInputValue(newInputValue);
                    }}
                    value={selectedOption}
                    onChange={(event, newStation) => {
                        // TODO Do not select if the locations search happened just now (~100ms?), to prevent misclicks
                        // which happen easily due to debounce
                        setOptions(newStation ? [newStation, ...options] : options);
                        setSelectedOption(newStation);
                    }}
                    sx={{ width: { sm: 450 } }}
                    renderInput={(params) => <TextField {...params} label="Station" placeholder="Search station" />}
                    getOptionLabel={(option) => option.name}
                    renderOption={renderOption}
                />
                <Button type="submit" variant="contained" disabled={isSubmissionDisabled}>
                    Save
                </Button>
            </Stack>
        </Dialog>
    );
}
