import HomeIcon from '@mui/icons-material/Home';
import InfoIcon from '@mui/icons-material/Info';
import MenuIcon from '@mui/icons-material/Menu';
import {
    AppBar,
    Box,
    CssBaseline,
    Drawer,
    IconButton,
    List,
    ListItem,
    ListItemButton,
    ListItemIcon,
    ListItemText,
    ThemeProvider,
    Toolbar,
    Typography,
    createTheme,
} from '@mui/material';
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
