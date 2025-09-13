import Alert from '@mui/material/Alert';
import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import CircularProgress from '@mui/material/CircularProgress';
import Paper from '@mui/material/Paper';
import Snackbar from '@mui/material/Snackbar';
import Stack from '@mui/material/Stack';
import TextField from '@mui/material/TextField';
import Typography from '@mui/material/Typography';
import { AxiosError } from 'axios';
import { useState, useEffect } from 'react';

import useSWR from 'swr';
import useSWRMutation from 'swr/mutation';
import { SettingsPostRequestSchema } from '../../api/Requests';
import { SettingsGetResponse } from '../../api/Responses';
import { Settings } from '../../Types';
import { getRequestSender, postRequestSender } from '../../util/Ajax';
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
    } = useSWR<SettingsGetResponse, AxiosError>('/api/settings', getRequestSender);
    const { trigger: triggerSettings, isMutating: isSettingsMutating } = useSWRMutation(
        '/api/settings',
        postRequestSender<SettingsPostRequestSchema>,
        { revalidate: false }
    );

    const [isStationChangeDialogOpen, setStationChangeDialogOpen] = useState(false);
    const [minDepartureMinutes, setMinDepartureMinutes] = useState<number>(0);
    const { state: snackbarState, openWithMessage: openSnackbarWithMessage, close: closeSnackbar } = useSnackbarState();

    // Sync local state with settings response
    useEffect(() => {
        if (settingsResponse) {
            setMinDepartureMinutes(settingsResponse.minDepartureMinutes);
        }
    }, [settingsResponse]);

    const handleSaveSettings = () => {
        void triggerSettings(
            {
                minDepartureMinutes,
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
                      }
                    : undefined,
            }
        );
    };

    const handleSaveCurrentStation = (newCurrentStation: NonNullable<Settings['currentStation']>) => {
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

                    <Paper sx={{ p: 3, mt: 3 }}>
                        <Typography variant="h5" gutterBottom>
                            Departure Filter Settings
                        </Typography>
                        <Typography variant="body2" sx={{ mb: 2, color: 'text.secondary' }}>
                            Configure the minimum time before departure to show connections. For example, if you set
                            this to 5 minutes, departures leaving in less than 5 minutes will be hidden.
                        </Typography>
                        <Stack direction={{ xs: 'column', sm: 'row' }} spacing={2} alignItems={{ sm: 'center' }}>
                            <TextField
                                label="Hide departures leaving in less than (minutes)"
                                type="number"
                                value={minDepartureMinutes}
                                onChange={(e) => {
                                    setMinDepartureMinutes(Math.max(0, Math.min(30, parseInt(e.target.value) || 0)));
                                }}
                                slotProps={{
                                    htmlInput: {
                                        min: 0,
                                        max: 30,
                                        step: 1,
                                    },
                                }}
                                sx={{ minWidth: { xs: '100%', sm: 300 } }}
                                disabled={isSettingsMutating || isSettingsValidating}
                                size="small"
                            />
                            <Button
                                variant="contained"
                                onClick={handleSaveSettings}
                                disabled={isSettingsMutating || isSettingsValidating}
                                sx={{ minWidth: 100 }}>
                                Save
                            </Button>
                        </Stack>
                    </Paper>
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
