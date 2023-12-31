import { css } from '@emotion/react';
import { Stack, Typography, Tooltip, Switch, Grid } from '@mui/material';
import { CurrentStationPostRequestSchema } from 'frontend/api/Requests';
import { CurrentStationGetResponse } from 'frontend/api/Responses';
import LineIcon from 'frontend/components/LineIcon';
import { LineProductType } from 'frontend/Types';

interface ServicesRowProps {
    product: LineProductType;
    lineNames: Array<string>;
    enabled: boolean;
    showToggle: boolean;
    onEnabledChange: (enabled: boolean) => void;
    isToggleDisabled: boolean;
    isLastEnabledToggle: boolean;
}

function ServicesProductRow({
    product,
    lineNames,
    enabled,
    onEnabledChange,
    isToggleDisabled,
    showToggle,
    isLastEnabledToggle,
}: ServicesRowProps) {
    return (
        <Stack direction={{ xs: 'column', md: 'row' }} gap={1} alignItems={{ md: 'center' }}>
            <Stack direction="row" alignItems="center" justifyContent="space-between">
                <Typography variant="body1" sx={{ minWidth: 100 }}>
                    {product.charAt(0).toUpperCase() + product.slice(1)}
                </Typography>
                {showToggle && (
                    <Tooltip
                        title={`Click to ${enabled ? 'disable' : 'enable'}`}
                        disableInteractive
                        PopperProps={{
                            popperOptions: {
                                modifiers: ['preventOverflow'],
                                strategy: 'fixed',
                            },
                        }}>
                        <Switch
                            checked={enabled}
                            disabled={isToggleDisabled || (enabled && isLastEnabledToggle)}
                            onChange={(event, checked) => {
                                onEnabledChange(checked);
                            }}
                        />
                    </Tooltip>
                )}
            </Stack>
            <Grid container key={product} columnGap={1} rowGap={1}>
                {lineNames.map((name) => (
                    // TODO Figure out what's the deal with the SVG size
                    <Grid
                        item
                        key={name}
                        css={css`
                            width: 60px;
                            height: 23px;
                        `}>
                        <LineIcon name={name} type={product} disabled={!enabled} fontSize="small" />
                    </Grid>
                ))}
            </Grid>
        </Stack>
    );
}

interface ServicesSectionProps {
    currentStation: NonNullable<CurrentStationGetResponse>;
    saveNewCurrentStation: (newCurrentStation: CurrentStationPostRequestSchema) => void;
    disableToggles: boolean;
}

export default function ServicesSection({
    currentStation,
    saveNewCurrentStation,
    disableToggles,
}: ServicesSectionProps) {
    const showToggle = Object.keys(currentStation.linesByProduct).length > 1;
    const areTogglesDisabled = disableToggles;
    const isLastEnabledToggle = currentStation.enabledProducts.length === 1;

    const toggleProductEnabled = (product: LineProductType, enabled: boolean) => {
        saveNewCurrentStation({
            ...currentStation,
            enabledProducts: enabled
                ? [...currentStation.enabledProducts, product]
                : currentStation.enabledProducts.filter((enabledProduct) => enabledProduct !== product),
        });
    };

    return (
        <>
            <Typography variant="h4" gutterBottom>
                Services
            </Typography>
            <Stack direction="column" gap={1}>
                {Object.entries(currentStation.linesByProduct).map(([product, lines]) => (
                    <ServicesProductRow
                        key={product}
                        product={product as LineProductType}
                        lineNames={lines}
                        enabled={currentStation.enabledProducts.includes(product as LineProductType)}
                        isToggleDisabled={areTogglesDisabled}
                        onEnabledChange={(enabled) => {
                            toggleProductEnabled(product as LineProductType, enabled);
                        }}
                        showToggle={showToggle}
                        isLastEnabledToggle={isLastEnabledToggle}
                    />
                ))}
            </Stack>
        </>
    );
}
