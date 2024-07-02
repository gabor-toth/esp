import {NgModule} from '@angular/core';
import {BrowserModule} from '@angular/platform-browser';

// import {AppRoutingModule} from './app-routing.module';
// import {AppComponent} from './app.component';

import {HttpClientModule} from '@angular/common/http';
import {PinComponent} from './pin/pin.component';
import {StateComponent} from './state/state.component';
import {AdminComponent} from './admin/admin.component';
import {ProgramsComponent} from './program/programs.component';

import {MatTableModule} from "@angular/material/table";
import {BrowserAnimationsModule} from '@angular/platform-browser/animations';
import {MatIconModule} from "@angular/material/icon";
import {MatButtonModule} from "@angular/material/button";
import {MatProgressSpinnerModule} from "@angular/material/progress-spinner";
// import {LayoutModule} from '@angular/cdk/layout';
import {FlexLayoutModule} from '@angular/flex-layout';

@NgModule({
    declarations: [
        // AppComponent,
        StateComponent,
        AdminComponent,
        ProgramsComponent,
        PinComponent,
    ],
    imports: [
        BrowserModule,
        // AppRoutingModule,
        HttpClientModule,
        BrowserAnimationsModule,
        MatTableModule,
        MatIconModule,
        MatButtonModule,
        MatProgressSpinnerModule,
        // https://material.angular.io/cdk/layout/overview
        // LayoutModule,
        FlexLayoutModule,
    ],
    providers: [],
    // bootstrap: [AppComponent]
})
export class AppModule {
}
