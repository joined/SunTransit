import { AlertColor } from '@mui/material/Alert';
import { useReducer } from 'react';

interface SnackbarReducerState {
    open: boolean;
    message: string;
    severity: AlertColor;
}

interface SnackbarReducerOpenAction {
    type: 'open';
    message: string;
    severity: AlertColor;
}

interface SnackbarReducerCloseAction {
    type: 'close';
}

type SnackbarReducerAction = SnackbarReducerOpenAction | SnackbarReducerCloseAction;

function snackbarReducer(state: SnackbarReducerState, action: SnackbarReducerAction): SnackbarReducerState {
    switch (action.type) {
        case 'open': {
            return {
                open: true,
                message: action.message,
                severity: action.severity,
            };
        }
        case 'close': {
            return {
                ...state,
                open: false,
            };
        }
    }
}

export function useSnackbarState() {
    const [state, dispatch] = useReducer(snackbarReducer, {
        open: false,
        message: '',
        severity: 'success',
    });

    return {
        state,
        openWithMessage: (message: string, severity: AlertColor) => {
            dispatch({
                type: 'open',
                message,
                severity,
            });
        },
        close: () => {
            dispatch({
                type: 'close',
            });
        },
    };
}
