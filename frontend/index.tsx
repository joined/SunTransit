import React from 'react';
import { createRoot } from 'react-dom/client';
import { RouterProvider, createBrowserRouter } from 'react-router-dom';
import { Root } from './Root';
import { HomeTab } from './tabs/HomeTab/HomeTab';
import { SystemInformationTab } from './tabs/SystemInformationTab/SystemInformationTab';

const router = createBrowserRouter([
    {
        path: '/',
        element: <Root />,
        children: [
            {
                index: true,
                element: <HomeTab />,
            },
            {
                path: '/sysinfo',
                element: <SystemInformationTab />,
            },
        ],
    },
]);

const rootNode = document.getElementById('root');

if (!rootNode) {
    throw new Error('Could not find root node to inject React into.');
}

const root = createRoot(rootNode);
root.render(
    <React.StrictMode>
        <RouterProvider router={router} />
    </React.StrictMode>
);
