import Alert from '@mui/material/Alert';
import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import CircularProgress from '@mui/material/CircularProgress';
import Snackbar from '@mui/material/Snackbar';
import Stack from '@mui/material/Stack';
import TextField from '@mui/material/TextField';
import Typography from '@mui/material/Typography';
import { AxiosError } from 'axios';
import { useState, useEffect } from 'react';

import useSWR from 'swr';
import useSWRMutation from 'swr/mutation';
import { SettingsRequest } from '../../api/Requests';
import { SettingsResponse } from '../../api/Responses';
import { StationWithProducts } from '../../Types';
import { getRequestSender, postRequestSender } from '../../util/Ajax';
import {
    MIN_DEPARTURE_MINUTES_MIN,
    MIN_DEPARTURE_MINUTES_MAX,
    MAX_DEPARTURE_COUNT_MIN,
    MAX_DEPARTURE_COUNT_MAX,
} from '../../util/Constants';
import ServicesSection from './ServicesSection';
import StationChangeDialog from './StationChangeDialog';
import { useSnackbarState } from './useSnackbarState';

interface InitialConfigurationProps {
    onButtonClick: () => void;
    disabled: boolean;
}

function InitialConfiguration({ onButtonClick, disabled }: InitialConfigurationProps) {
    return (
        <Box>
            <Typography variant="body1" gutterBottom>
                No station selected. Please select a station to show departures from.
            </Typography>
            <Button disabled={disabled} sx={{ mt: 1 }} variant="contained" onClick={onButtonClick}>
                Select station
            </Button>
        </Box>
    );
}

export function HomeTab() {
    const {
        data: settingsResponse,
        error: settingsError,
        isLoading: isSettingsLoading,
        isValidating: isSettingsValidating,
    } = useSWR<SettingsResponse, AxiosError>('/api/settings', getRequestSender);
    const { trigger: triggerSettings, isMutating: isSettingsMutating } = useSWRMutation(
        '/api/settings',
        postRequestSender<SettingsRequest>,
        { revalidate: false }
    );

    const [isStationChangeDialogOpen, setStationChangeDialogOpen] = useState(false);
    const [minDepartureMinutes, setMinDepartureMinutes] = useState<number | undefined>(undefined);
    const [maxDepartureCount, setMaxDepartureCount] = useState<number | undefined>(undefined);
    const { state: snackbarState, openWithMessage: openSnackbarWithMessage, close: closeSnackbar } = useSnackbarState();

    // Sync local state with settings response
    useEffect(() => {
        if (settingsResponse) {
            setMinDepartureMinutes(settingsResponse.minDepartureMinutes);
            setMaxDepartureCount(settingsResponse.maxDepartureCount);
        }
    }, [settingsResponse]);

    const handleSaveSettings = () => {
        void triggerSettings(
            {
                minDepartureMinutes,
                maxDepartureCount,
            },
            {
                onSuccess: () => {
                    openSnackbarWithMessage('Settings saved successfully', 'success');
                },
                onError: () => {
                    openSnackbarWithMessage('Error saving settings, please try again', 'error');
                },
                optimisticData: settingsResponse
                    ? {
                          ...settingsResponse,
                          minDepartureMinutes,
                          maxDepartureCount,
                      }
                    : undefined,
            }
        );
    };

    const handleSaveCurrentStation = (newCurrentStation: StationWithProducts) => {
        void triggerSettings(
            {
                currentStation: newCurrentStation,
                minDepartureMinutes: 0, // Reset filter when station changes
            },
            {
                onSuccess: () => {
                    openSnackbarWithMessage('Station changed successfully', 'success');
                    setMinDepartureMinutes(0); // Update local state
                },
                onError: () => {
                    openSnackbarWithMessage('Error, please try again', 'error');
                },
                optimisticData: settingsResponse
                    ? {
                          ...settingsResponse,
                          currentStation: newCurrentStation,
                          minDepartureMinutes: 0,
                      }
                    : undefined,
            }
        );
    };

    if (isSettingsLoading) {
        return <CircularProgress color="secondary" />;
    }

    if (settingsError) {
        return <p>Error loading settings.</p>;
    }

    return (
        <Box maxWidth={800}>
            <Typography variant="h3" gutterBottom>
                Departures panel configuration
            </Typography>
            {!settingsResponse?.currentStation ? (
                <InitialConfiguration
                    disabled={isSettingsValidating}
                    onButtonClick={() => {
                        setStationChangeDialogOpen(true);
                    }}
                />
            ) : (
                <Box>
                    <Stack
                        direction={{ xs: 'column', sm: 'row' }}
                        justifyContent={{ sm: 'space-between' }}
                        alignItems={{ sm: 'center' }}
                        gap={{ xs: 1, sm: 2 }}
                        marginBottom={2}>
                        <Stack direction={{ xs: 'column', md: 'row' }} gap={1}>
                            <Box>Currently selected station:</Box>
                            <Box sx={{ fontWeight: 'bold' }} display="inline">
                                {settingsResponse.currentStation.name}
                            </Box>
                        </Stack>
                        <Button
                            disabled={isSettingsValidating || isSettingsMutating}
                            sx={{ width: { xs: 100, sm: 'auto' } }}
                            variant="contained"
                            onClick={() => {
                                setStationChangeDialogOpen(true);
                            }}>
                            Change
                        </Button>
                    </Stack>
                    <ServicesSection
                        currentStation={settingsResponse.currentStation}
                        saveNewCurrentStation={handleSaveCurrentStation}
                        disableToggles={isSettingsMutating || isSettingsValidating}
                    />

                    <Typography variant="h4" gutterBottom sx={{ mt: 3, mb: 2 }}>
                        Settings
                    </Typography>
                    <Stack spacing={3}>
                        <TextField
                            label={`Maximum number of departures to show (${MAX_DEPARTURE_COUNT_MIN.toString()}-${MAX_DEPARTURE_COUNT_MAX.toString()})`}
                            type="number"
                            value={maxDepartureCount ?? ''}
                            onChange={(e) => {
                                setMaxDepartureCount(parseInt(e.target.value));
                            }}
                            slotProps={{
                                htmlInput: {
                                    min: MAX_DEPARTURE_COUNT_MIN,
                                    max: MAX_DEPARTURE_COUNT_MAX,
                                    step: 1,
                                },
                            }}
                            sx={{ maxWidth: 360 }}
                            disabled={isSettingsMutating || isSettingsValidating || maxDepartureCount === undefined}
                            size="small"
                        />
                        <TextField
                            label={`Hide departures leaving sooner than this (${MIN_DEPARTURE_MINUTES_MIN.toString()}-${MIN_DEPARTURE_MINUTES_MAX.toString()} min)`}
                            type="number"
                            value={minDepartureMinutes ?? ''}
                            onChange={(e) => {
                                setMinDepartureMinutes(parseInt(e.target.value));
                            }}
                            slotProps={{
                                htmlInput: {
                                    min: MIN_DEPARTURE_MINUTES_MIN,
                                    max: MIN_DEPARTURE_MINUTES_MAX,
                                    step: 1,
                                },
                            }}
                            sx={{ maxWidth: 360 }}
                            disabled={isSettingsMutating || isSettingsValidating || minDepartureMinutes === undefined}
                            size="small"
                        />
                        <Button
                            variant="contained"
                            onClick={handleSaveSettings}
                            disabled={
                                isSettingsMutating ||
                                isSettingsValidating ||
                                minDepartureMinutes === undefined ||
                                maxDepartureCount === undefined
                            }
                            sx={{ alignSelf: 'flex-start', minWidth: 100 }}>
                            Save Settings
                        </Button>
                    </Stack>
                </Box>
            )}
            <StationChangeDialog
                currentStationId={settingsResponse?.currentStation?.id ?? null}
                open={isStationChangeDialogOpen}
                saveNewCurrentStation={(newCurrentStation, onSuccess) => {
                    handleSaveCurrentStation(newCurrentStation);
                    onSuccess();
                    setStationChangeDialogOpen(false);
                }}
                onClose={() => {
                    setStationChangeDialogOpen(false);
                }}
                isMutating={isSettingsMutating}
            />
            <Snackbar open={snackbarState.open} autoHideDuration={3000} onClose={closeSnackbar}>
                <Alert severity={snackbarState.severity} variant="filled">
                    {snackbarState.message}
                </Alert>
            </Snackbar>
        </Box>
    );
}
