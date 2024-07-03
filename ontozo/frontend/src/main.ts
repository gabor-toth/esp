import { enableProdMode } from '@angular/core';
import { bootstrapApplication } from '@angular/platform-browser';

import { environment } from './environments/environment';

if ( environment.production ) {
  enableProdMode();
}

// AppModule import is needed, it loads the Material stuff
import { AppModule } from './app/app.module';
import { appConfig } from './app/app.config';
import { AppComponent } from './app/app.component';

bootstrapApplication( AppComponent, appConfig )
  .catch( ( err ) => console.error( err ) );
