import HomeIcon from '@mui/icons-material/Home';
import InfoIcon from '@mui/icons-material/Info';
import MenuIcon from '@mui/icons-material/Menu';
import PhonelinkEraseIcon from '@mui/icons-material/PhonelinkErase';
import AppBar from '@mui/material/AppBar';
import Box from '@mui/material/Box';
import CssBaseline from '@mui/material/CssBaseline';
import Drawer from '@mui/material/Drawer';
import IconButton from '@mui/material/IconButton';
import List from '@mui/material/List';
import ListItem from '@mui/material/ListItem';
import ListItemButton from '@mui/material/ListItemButton';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';
import { createTheme, ThemeProvider } from '@mui/material/styles';
import Toolbar from '@mui/material/Toolbar';
import Typography from '@mui/material/Typography';
import { useState } from 'react';

import { Outlet, useLocation, useNavigate } from 'react-router';
import { RouteConfig } from './Types';
import { DRAWER_WIDTH } from './util/Constants';

const theme = createTheme({
    palette: {
        mode: 'dark',
    },
});

export const ROUTES: Array<RouteConfig> = [
    {
        path: '/',
        name: 'Home',
        drawerIcon: <HomeIcon />,
    },
    {
        path: '/sysinfo',
        name: 'System information',
        drawerIcon: <InfoIcon />,
    },
];

const NavigationDrawerContent = ({ onNavigation }: { onNavigation?: VoidFunction }) => {
    const navigate = useNavigate();
    const { pathname } = useLocation();

    const handleClearMockStorage = async () => {
        if (process.env.NODE_ENV === 'development') {
            const { clearMockStorage } = await import('./mocks/handlers');
            clearMockStorage();
        }
    };

    return (
        <List>
            {ROUTES.map((route) => (
                <ListItem key={route.path} disablePadding>
                    <ListItemButton
                        selected={pathname === route.path}
                        onClick={() => {
                            onNavigation?.();
                            void navigate(route.path);
                        }}>
                        <ListItemIcon>{route.drawerIcon}</ListItemIcon>
                        <ListItemText primary={route.name} />
                    </ListItemButton>
                </ListItem>
            ))}
            {process.env.NODE_ENV === 'development' && (
                <ListItem disablePadding>
                    <ListItemButton
                        onClick={() => {
                            void handleClearMockStorage();
                        }}>
                        <ListItemIcon>
                            <PhonelinkEraseIcon />
                        </ListItemIcon>
                        <ListItemText primary="Clear Mock Storage" />
                    </ListItemButton>
                </ListItem>
            )}
        </List>
    );
};

export const Root = () => {
    const [mobileOpen, setMobileOpen] = useState(false);

    return (
        <ThemeProvider theme={theme}>
            <Box sx={{ display: 'flex' }}>
                <CssBaseline />
                <AppBar
                    position="fixed"
                    sx={{
                        width: { sm: `calc(100% - ${DRAWER_WIDTH.toString()}px)` },
                        ml: { sm: `${DRAWER_WIDTH.toString()}px` },
                    }}>
                    <Toolbar>
                        <IconButton
                            color="inherit"
                            edge="start"
                            onClick={() => {
                                setMobileOpen(true);
                            }}
                            sx={{ mr: 2, display: { sm: 'none' } }}>
                            <MenuIcon />
                        </IconButton>
                        {/* TODO On mobile show the tab name? */}
                        <Typography variant="h6" noWrap component="div">
                            SunTransit
                        </Typography>
                    </Toolbar>
                </AppBar>
                <Box component="nav" sx={{ width: { sm: DRAWER_WIDTH }, flexShrink: { sm: 0 } }}>
                    <Drawer
                        variant="temporary"
                        open={mobileOpen}
                        onClose={() => {
                            setMobileOpen(false);
                        }}
                        ModalProps={{
                            keepMounted: true,
                        }}
                        sx={{
                            display: { xs: 'block', sm: 'none' },
                            '& .MuiDrawer-paper': { boxSizing: 'border-box', width: DRAWER_WIDTH },
                        }}>
                        <NavigationDrawerContent
                            onNavigation={() => {
                                setMobileOpen(false);
                            }}
                        />
                    </Drawer>
                    <Drawer
                        variant="permanent"
                        sx={{
                            display: { xs: 'none', sm: 'block' },
                            '& .MuiDrawer-paper': { boxSizing: 'border-box', width: DRAWER_WIDTH },
                        }}
                        open>
                        <NavigationDrawerContent />
                    </Drawer>
                </Box>
                <Box
                    component="main"
                    sx={{ flexGrow: 1, p: 3, width: { sm: `calc(100% - ${DRAWER_WIDTH.toString()}px)` } }}>
                    <Toolbar />
                    <Outlet />
                </Box>
            </Box>
        </ThemeProvider>
    );
};
