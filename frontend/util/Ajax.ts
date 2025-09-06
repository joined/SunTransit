import axios from 'axios';

export const getRequestSender = <TResponse>(url: string) => axios.get<TResponse>(url).then((res) => res.data);
// TODO Figure out if we can do better here
// eslint-disable-next-line @typescript-eslint/no-unnecessary-type-parameters
export const postRequestSender = <TRequestBody>(url: string, { arg }: { arg: TRequestBody }) =>
    axios.post(url, { ...arg });
