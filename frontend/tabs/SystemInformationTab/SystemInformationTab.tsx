import { css } from '@emotion/react';
import AlarmOnIcon from '@mui/icons-material/AlarmOn';
import BlockIcon from '@mui/icons-material/Block';
import DeleteIcon from '@mui/icons-material/Delete';
import DirectionsRunIcon from '@mui/icons-material/DirectionsRun';
import PauseIcon from '@mui/icons-material/Pause';
import QuestionMarkIcon from '@mui/icons-material/QuestionMark';
import Box from '@mui/material/Box';
import Checkbox from '@mui/material/Checkbox';
import CircularProgress from '@mui/material/CircularProgress';
import FormControlLabel from '@mui/material/FormControlLabel';
import FormGroup from '@mui/material/FormGroup';
import Link from '@mui/material/Link';
import Paper from '@mui/material/Paper';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import Typography from '@mui/material/Typography';
import { AxiosError } from 'axios';
import React, { useState } from 'react';
import * as R from 'remeda';
import useSWRImmutable from 'swr/immutable';
import {
    SysInfoResponse,
    SysInfoSoftwareResponse,
    SysInfoMemoryResponse,
    SysInfoHardwareResponse,
    SysInfoTaskResponse,
    SysInfoAppStateResponse,
    SysInfoDebugResponse,
} from '../../api/Responses';
import { getRequestSender } from '../../util/Ajax';
import { SYS_INFO_REFRESH_INTERVAL } from '../../util/Constants';

const TASK_STATUS_TO_ICON: Record<number, React.ReactElement> = {
    0: <DirectionsRunIcon />, // Running
    1: <AlarmOnIcon />, // Ready
    2: <BlockIcon />, // Blocked
    3: <PauseIcon />, // Suspended
    4: <DeleteIcon />, // Deleted
    5: <QuestionMarkIcon />, // Invalid
};

const CHIP_MODEL_TO_NAME: Record<number, string> = {
    1: 'ESP32',
    2: 'ESP32-S2',
    5: 'ESP32-C3',
    9: 'ESP32-S3',
    12: 'ESP32-C2',
    13: 'ESP32-C6',
    16: 'ESP32-H2',
    999: 'POSIX/Linux simulator',
};

const KEY_TO_LABEL: Record<string, string> = {
    time: 'System time (UTC)',
    app_version: 'App version',
    mdns_hostname: 'mDNS hostname',
    idf_version: 'IDF version',
    project_name: 'Project name',
    compile_time: 'Compile time',
    compile_date: 'Compile date',
    free_heap: 'Free heap',
    minimum_free_heap: 'Minimum free heap since boot',
    mac_address: 'MAC address',
    chip_model: 'Chip model',
    bvg_api_url: 'BVG API URL',
};

const bottomMarginStyle = css`
    margin-bottom: 16px;
`;
const lastTableRowStyle = css`
    &:last-child td,
    &:last-child th {
        border: 0;
    }
`;

const AppStateTable = ({ data }: { data: SysInfoAppStateResponse }) => (
    <TableContainer component={Paper} css={bottomMarginStyle}>
        <Table>
            <TableBody>
                <TableRow key={'time'} css={lastTableRowStyle}>
                    <TableCell component="th" scope="row">
                        {KEY_TO_LABEL.time}
                    </TableCell>
                    <TableCell align="right">{data.time ? new Date(data.time).toISOString() : 'Not set'}</TableCell>
                </TableRow>
                <TableRow key={'mdns_hostname'} css={lastTableRowStyle}>
                    <TableCell component="th" scope="row">
                        {KEY_TO_LABEL.mdns_hostname}
                    </TableCell>
                    <TableCell align="right">{data.mdns_hostname.toLowerCase()}</TableCell>
                </TableRow>
            </TableBody>
        </Table>
    </TableContainer>
);

const SoftwareTable = ({ data }: { data: SysInfoSoftwareResponse }) => (
    <TableContainer component={Paper} css={bottomMarginStyle}>
        <Table>
            <TableBody>
                {(
                    ['app_version', 'idf_version', 'project_name', 'compile_time', 'compile_date'] satisfies Array<
                        keyof SysInfoSoftwareResponse
                    >
                ).map((key) => (
                    <TableRow key={key} css={lastTableRowStyle}>
                        <TableCell component="th" scope="row">
                            {KEY_TO_LABEL[key] || key}
                        </TableCell>
                        <TableCell align="right">{data[key]}</TableCell>
                    </TableRow>
                ))}
            </TableBody>
        </Table>
    </TableContainer>
);

const MemoryTable = ({ data }: { data: SysInfoMemoryResponse }) => (
    <TableContainer component={Paper} css={bottomMarginStyle}>
        <Table>
            <TableBody>
                {(['free_heap', 'minimum_free_heap'] satisfies Array<keyof SysInfoMemoryResponse>).map((key) => (
                    <TableRow key={key} css={lastTableRowStyle}>
                        <TableCell component="th" scope="row">
                            {KEY_TO_LABEL[key] || key}
                        </TableCell>
                        <TableCell align="right">{data[key]} bytes</TableCell>
                    </TableRow>
                ))}
            </TableBody>
        </Table>
    </TableContainer>
);

const HardwareTable = ({ data }: { data: SysInfoHardwareResponse }) => (
    // TODO Maybe use small variant of the table when there's little space?
    <TableContainer component={Paper} css={bottomMarginStyle}>
        <Table>
            <TableBody>
                <TableRow key={'chip_model'} css={lastTableRowStyle}>
                    <TableCell component="th" scope="row">
                        {KEY_TO_LABEL.chip_model}
                    </TableCell>
                    <TableCell align="right">{CHIP_MODEL_TO_NAME[data.chip_model]}</TableCell>
                </TableRow>
                <TableRow key={'mac_address'} css={lastTableRowStyle}>
                    <TableCell component="th" scope="row">
                        {KEY_TO_LABEL.mac_address}
                    </TableCell>
                    <TableCell align="right">{data.mac_address}</TableCell>
                </TableRow>
            </TableBody>
        </Table>
    </TableContainer>
);

const DebugTable = ({ data }: { data: SysInfoDebugResponse }) => (
    <TableContainer component={Paper} css={bottomMarginStyle}>
        <Table>
            <TableBody>
                <TableRow key={'bvg_api_url'} css={lastTableRowStyle}>
                    <TableCell component="th" scope="row">
                        {KEY_TO_LABEL.bvg_api_url}
                    </TableCell>
                    <TableCell
                        align="right"
                        css={css`
                            word-break: break-all;
                        `}>
                        {data.bvg_api_url ? (
                            <Link href={data.bvg_api_url} target="_blank" rel="noopener noreferrer" color="primary">
                                {data.bvg_api_url}
                            </Link>
                        ) : (
                            'No station configured'
                        )}
                    </TableCell>
                </TableRow>
            </TableBody>
        </Table>
    </TableContainer>
);

const TaskTable = ({ data }: { data: Array<SysInfoTaskResponse> }) => (
    <TableContainer component={Paper} css={bottomMarginStyle}>
        <Table>
            <TableHead>
                <TableRow>
                    <TableCell>Name</TableCell>
                    <TableCell align="right">Priority</TableCell>
                    <TableCell align="right">Stack high watermark</TableCell>
                    <TableCell align="right">State</TableCell>
                    {data[0].runtime !== null ? <TableCell align="right">Runtime</TableCell> : null}
                    {data[0].core_id !== null ? <TableCell align="right">Core ID</TableCell> : null}
                </TableRow>
            </TableHead>
            <TableBody>
                {R.sortBy(data, (task) => -task.stack_high_water_mark).map((task) => (
                    <TableRow
                        key={`${task.name}-${task.core_id ? task.core_id.toString() : '?'}`}
                        css={lastTableRowStyle}>
                        <TableCell
                            css={css`
                                word-break: break-all;
                            `}>
                            {task.name}
                        </TableCell>
                        <TableCell align="right">{task.priority}</TableCell>
                        <TableCell align="right">{task.stack_high_water_mark} bytes</TableCell>
                        <TableCell align="right">{TASK_STATUS_TO_ICON[task.state]}</TableCell>
                        {task.runtime !== null ? <TableCell align="right">{task.runtime}%</TableCell> : null}
                        {task.core_id !== null ? <TableCell align="right">{task.core_id}</TableCell> : null}
                    </TableRow>
                ))}
            </TableBody>
        </Table>
    </TableContainer>
);

export const SystemInformationTab = () => {
    const [isAutoRefreshEnabled, setAutoRefreshEnabled] = useState(false);
    const { data, error, isLoading } = useSWRImmutable<SysInfoResponse, AxiosError>('/api/sysinfo', getRequestSender, {
        refreshInterval: isAutoRefreshEnabled ? SYS_INFO_REFRESH_INTERVAL : 0,
    });

    if (isLoading || !data) {
        return <CircularProgress color="secondary" />;
    }

    if (error) {
        return <p>Error loading system information.</p>;
    }

    return (
        <Box sx={{ maxWidth: 800 }}>
            <Typography variant="h3" gutterBottom>
                System information
            </Typography>
            <FormGroup
                css={css`
                    margin-bottom: 16px;
                `}>
                <FormControlLabel
                    control={
                        <Checkbox
                            checked={isAutoRefreshEnabled}
                            onChange={(event) => {
                                setAutoRefreshEnabled(event.target.checked);
                            }}
                        />
                    }
                    label={`Auto-refresh every ${Math.floor(SYS_INFO_REFRESH_INTERVAL / 1000).toString()}s`}
                />
            </FormGroup>
            <Typography variant="h4" gutterBottom>
                App state
            </Typography>
            <AppStateTable data={data.app_state} />
            <Typography variant="h4" gutterBottom>
                Software
            </Typography>
            <SoftwareTable data={data.software} />
            <Typography variant="h4" gutterBottom>
                Memory
            </Typography>
            <MemoryTable data={data.memory} />
            <Typography variant="h4" gutterBottom>
                Hardware
            </Typography>
            <HardwareTable data={data.hardware} />
            <Typography variant="h4" gutterBottom>
                Debug
            </Typography>
            <DebugTable data={data.debug} />
            {data.tasks ? (
                <>
                    <Typography variant="h4" gutterBottom>
                        FreeRTOS Tasks
                    </Typography>
                    {/* TODO This wrapper div is weird */}
                    <div
                        css={css`
                            max-width: calc(100vw - 64px);
                        `}>
                        <TaskTable data={data.tasks} />
                    </div>
                </>
            ) : (
                <p>Task trace facility disabled.</p>
            )}
        </Box>
    );
};
