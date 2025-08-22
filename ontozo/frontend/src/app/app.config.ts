import { ApplicationConfig, provideBrowserGlobalErrorListeners, provideZoneChangeDetection } from '@angular/core';
import { provideAnimationsAsync } from '@angular/platform-browser/animations/async';
import { HTTP_INTERCEPTORS, provideHttpClient, withInterceptors, withInterceptorsFromDi } from "@angular/common/http";
import { provideRouter } from '@angular/router';
import { routes } from './app.routes';
import { DEFAULT_TIMEOUT, TimeoutInterceptor } from "./common/timeout.interceptor";
import { environment } from "../environments/environment";

export const appConfig: ApplicationConfig = {
  providers: [
    provideBrowserGlobalErrorListeners(),
    provideZoneChangeDetection({ eventCoalescing: true }),
    provideAnimationsAsync(),
    provideHttpClient( withInterceptorsFromDi() ),
    provideRouter( routes ),
    [ { provide: HTTP_INTERCEPTORS, useClass: TimeoutInterceptor, multi: true } ],
    [ { provide: DEFAULT_TIMEOUT, useValue: environment.defaultHttpTimeout } ],
  ]
};
