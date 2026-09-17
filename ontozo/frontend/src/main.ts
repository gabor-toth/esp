import { enableProdMode } from '@angular/core';
import { bootstrapApplication } from '@angular/platform-browser';

import { environment } from './environments/environment';

if ( environment.production ) {
  enableProdMode();
}

import { appConfig } from './app/app.config';
import { App } from './app/app';

bootstrapApplication( App, appConfig )
  .catch( ( err ) => console.error( err ) );
