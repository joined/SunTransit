import { render, screen } from '@testing-library/react';
import LineIcon from '../components/LineIcon';
import { LineProductType } from '../Types';

describe('LineIcon Component', () => {
    it('renders line name correctly', () => {
        render(<LineIcon name="U6" type="subway" />);
        expect(screen.getByText('U6')).toBeInTheDocument();
    });

    it('applies correct styles for different transport types', () => {
        const testCases: Array<{ type: LineProductType; expectedColor: string }> = [
            { type: 'bus', expectedColor: 'var(--purple)' },
            { type: 'tram', expectedColor: 'var(--tram-red)' },
            { type: 'suburban', expectedColor: 'var(--green)' },
            { type: 'subway', expectedColor: 'var(--blue)' },
            { type: 'ferry', expectedColor: 'var(--blue)' },
            { type: 'express', expectedColor: 'var(--db-red)' },
            { type: 'regional', expectedColor: 'var(--db-red)' },
        ];

        testCases.forEach(({ type, expectedColor }) => {
            const { container } = render(<LineIcon name="Test" type={type} />);
            const rect = container.querySelector('rect');
            expect(rect).toHaveStyle(`fill: ${expectedColor}`);
        });
    });

    it('applies disabled styling when disabled prop is true', () => {
        const { container } = render(<LineIcon name="M10" type="tram" disabled />);
        const rect = container.querySelector('rect');
        const text = container.querySelector('text');
        
        expect(rect).toHaveStyle('opacity: 0.3');
        expect(text).toHaveStyle('opacity: 0.3');
    });

    it('applies correct font size for small variant', () => {
        const { container } = render(<LineIcon name="S1" type="suburban" fontSize="small" />);
        const text = container.querySelector('text');
        expect(text).toHaveStyle('font-size: 41px');
    });

    it('applies default medium font size when fontSize prop is not provided', () => {
        const { container } = render(<LineIcon name="S1" type="suburban" />);
        const text = container.querySelector('text');
        expect(text).toHaveStyle('font-size: 50px');
    });
});
