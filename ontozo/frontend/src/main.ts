import { bootstrapApplication } from '@angular/platform-browser';
import { enableProdMode } from '@angular/core';

import { environment } from './environments/environment';
import { provideAnimationsAsync } from '@angular/platform-browser/animations/async';

if (environment.production) {
  enableProdMode();
}

import { AppModule } from './app/app.module';
import { appConfig } from './app/app.config';
import { AppComponent } from './app/app.component';

bootstrapApplication(AppComponent, appConfig)
  .catch((err) => console.error(err));
